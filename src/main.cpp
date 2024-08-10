#include <Geode/Geode.hpp>
#include <map>

using namespace geode::prelude;

class XLevelCell;

namespace RTAGlobal {
	bool inRecentTab = false;
	bool loadingUnknown = false;

	CCContentLayer* currentContentLayer;
	float contentHeight;

	struct LevelObject {
		GJGameLevel* level;
		CCNode* cell;
		int childIndex;
		int level_id;
	};

	struct Account19 {
		int userID;
		int accountID;
		std::string username;
	};

	std::vector<LevelObject> deletedLevels;
	int deletedLevelsSize = 0;

	std::vector<std::string> splitString(const char* str, char d, unsigned int max_entries = 0) {
		std::vector<std::string> result;

		do {
			const char* begin = str;

			while (*str != d && *str) str++;

			result.push_back(std::string(begin, str));
		} while (0 != *str++);

		return result;
	}

	std::map<int, std::string> parseObjectData(std::string& object_string, char delim = ',') {
		std::vector<std::string> data = splitString(object_string.data(), delim);

		std::map<int, std::string> object_map;

		bool _key = true;

		int key;
		std::string value;

		for (std::string el : data) {
			if (_key) {
				key = std::stoi(el);
			}
			else {
				value = el;

				object_map[key] = value;
			}

			_key = !_key;
		}

		return object_map;
	}
}

#include <Geode/modify/LevelBrowserLayer.hpp>
#include <Geode/modify/LevelCell.hpp>

class $modify(XLevelCell, LevelCell) {
	struct Fields {
		LoadingCircleSprite* circle = nullptr;
	};

	bool init() {
		if (!LevelCell::init()) return false;

		if (!RTAGlobal::loadingUnknown) return true;

		return true;
	}

	void doFail() {
		m_fields->circle->setVisible(false);

		CCLabelBMFont* text = CCLabelBMFont::create("Deleted or friends-only level", "bigFont.fnt", getContentWidth() / 1.25f);
		auto size = getContentSize();

		text->setPosition(size / 2.f);
		text->setScale(0.5f);

		addChild(text);
	}

	void setupHiddenLoader(int level_id) {
		m_fields->circle = LoadingCircleSprite::create();
		m_fields->circle->hideCircle();
		m_fields->circle->fadeInCircle(false);

		auto size = getContentSize();

		addChild(m_fields->circle);

		m_fields->circle->setPosition(size / 2.f);

		RTAGlobal::LevelObject lobj = { nullptr, this, 0, level_id };
		RTAGlobal::deletedLevels.push_back(lobj);
	}

	CCNode* getTargetFromButton(CCMenuItemSpriteExtra* btn) {
		return btn->getNormalImage();
	}

	void betterInfoFix() {
		CCNode* idLabelNd = getChildByIDRecursive("cvolton.betterinfo/level-id-label");

		if (!idLabelNd) return;

		CCLabelBMFont* idLabel = typeinfo_cast<CCLabelBMFont*>(idLabelNd);

		if (!idLabel) return;

		idLabel->setColor({ 255, 255, 255 });
	}

	void fixLayers() {
		m_mainLayer->setContentSize(getContentSize());

		auto authorLabelBtn = typeinfo_cast<CCMenuItemSpriteExtra*>(getChildByIDRecursive("creator-name"));
		CCLabelBMFont* authorLabel = typeinfo_cast<CCLabelBMFont*>(getTargetFromButton(authorLabelBtn));
		float scale = authorLabel->getScale();
		std::string str = authorLabel->getString();
		auto color = authorLabel->getColor();

		m_mainMenu->removeMeAndCleanup();

		authorLabel = CCLabelBMFont::create(str.c_str(), "goldFont.fnt");
		authorLabel->setColor(color);
		authorLabel->setScale(scale);

		m_mainMenu = CCMenu::create();
		m_mainMenu->setContentSize(getContentSize());
		m_mainMenu->setID("main-menu");
		m_mainMenu->setPosition({ 0, 0 });

		addChild(m_mainMenu);

		authorLabelBtn = CCMenuItemSpriteExtra::create(authorLabel, this, menu_selector(LevelCell::onViewProfile));
		authorLabelBtn->setID("creator-name");
		authorLabelBtn->setPosition({ 118, 53 });

		ButtonSprite* getItSpr = ButtonSprite::create("Get It", 55, true, "bigFont.fnt", "GJ_button_02.png", 29, 0.5f);
		CCMenuItemSpriteExtra* getItBtn = CCMenuItemSpriteExtra::create(getItSpr, this, menu_selector(LevelCell::onClick));
		getItBtn->setID("view-button");
		getItBtn->setPositionX(getContentWidth() - (getItBtn->getContentWidth() / 2.f) - 9.f);
		getItBtn->setPositionY(getContentHeight() / 2.f);

		m_mainMenu->addChild(authorLabelBtn);
		m_mainMenu->addChild(getItBtn);
	}

	void loadFromLevel(GJGameLevel* level) {
		if (level == nullptr && m_fields->circle != nullptr) {
			doFail();

			return;
		}

		if (level == nullptr && RTAGlobal::loadingUnknown) {
			int id = std::stoi(getID());
				
			setupHiddenLoader(id);

			return;
		}

		if (m_fields->circle != nullptr) {
			m_fields->circle->setVisible(false);

			LevelCell::loadFromLevel(level);
		
			// getPositionY() * getAnchorPoint().y
			fixLayers();
			betterInfoFix();
			setID(std::to_string(level->m_levelID));
		}
		else {
			LevelCell::loadFromLevel(level);
		}
	}

	void loadFromString(const std::string &remote_level_string) {
		GJGameLevel* level = GJGameLevel::create();
		level->retain();
		level->m_levelName = "test";
		level->m_levelID = 666;
		level->m_levelNotDownloaded = true;
		level->m_audioTrack = 1;
		level->m_userID = 1000;
		level->m_accountID = 1000;
		level->m_creatorName = "idk";
	}

	void onClick(CCObject* sender) {
		if (m_fields->circle != nullptr) {
			auto scene = LevelInfoLayer::scene(m_level, false);
			auto transition = CCTransitionFade::create(0.5f, scene);

			CCDirector::sharedDirector()->pushScene(transition);

			return;
		}

		LevelCell::onClick(sender);
	}
};

#include <Geode/utils/web.hpp>

class $modify(XLevelBrowserLayer, LevelBrowserLayer) {
public:
	struct Fields {
		CCContentLayer* list_entries_node;
		EventListener<utils::web::WebTask> m_listener;
	};

	bool init(GJSearchObject * obj) {
		RTAGlobal::inRecentTab = false;

		if (obj->m_searchType == SearchType::Recent) {
			RTAGlobal::inRecentTab = true;
		}

		return LevelBrowserLayer::init(obj);
	}

	void onGoToPage(CCObject* sender) { RTAGlobal::deletedLevels.clear(); /* log::info("cleared"); */ LevelBrowserLayer::onGoToPage(sender); }
	void onGoToLastPage(CCObject* sender) { RTAGlobal::deletedLevels.clear(); /* log::info("cleared"); */ LevelBrowserLayer::onGoToLastPage(sender); }
	void onNextPage(CCObject* sender) { RTAGlobal::deletedLevels.clear(); /* log::info("cleared"); */ LevelBrowserLayer::onNextPage(sender); }
	void onPrevPage(CCObject* sender) { RTAGlobal::deletedLevels.clear(); /* log::info("cleared"); */ LevelBrowserLayer::onPrevPage(sender); }
	void onRefresh(CCObject* sender) { RTAGlobal::deletedLevels.clear(); /* log::info("cleared"); */ LevelBrowserLayer::onRefresh(sender); }

	void keyBackClicked() {
		RTAGlobal::deletedLevels.clear(); 
		// log::info("cleared");

		LevelBrowserLayer::keyBackClicked();
	}

	void loadLevelsFinished(CCArray* p0, const char* p1, int p2) {
		// log::info("p0={} || p1={} || p2={}", p0, p1, p2);
		RTAGlobal::deletedLevels.clear();
		RTAGlobal::deletedLevelsSize = 0;

		LevelBrowserLayer::loadLevelsFinished(p0, p1, p2);

		if (!RTAGlobal::inRecentTab) {
			// log::info("error 1");

			return;
		}
		if (p0->count() == 0) {
			log::error("no levels available for analysys");

			return;
		}

		m_list->m_listView->m_tableView->sortAllChildren();
		CCContentLayer *list_entries_node = typeinfo_cast<CCContentLayer*>(m_list->m_listView->m_tableView->getChildren()->objectAtIndex(0));

		RTAGlobal::currentContentLayer = list_entries_node;

		if (!list_entries_node) {
			log::error("CCContentLayer cannot be found");

			return;
		}

		//  log::info("list_entries_node={}", list_entries_node);

		list_entries_node->sortAllChildren();
		CCArray* children = list_entries_node->getChildren();

		int first_level_id = typeinfo_cast<LevelCell*>(children->objectAtIndex(0))->m_level->m_levelID;

		std::map<int, RTAGlobal::LevelObject> serverMap;
		std::map<int, RTAGlobal::LevelObject> actualMap;
		std::vector<int> targets;

		int cell_height = 90.f;

		for (int i = 0; i < children->count(); i++) {
			CCObject* cellRawObject = children->objectAtIndex(i);
			LevelCell* levelObject = typeinfo_cast<LevelCell*>(cellRawObject);

			cell_height = levelObject->getContentHeight();

			serverMap[levelObject->m_level->m_levelID] = { levelObject->m_level, levelObject, i, levelObject->m_level->m_levelID };
		}

		for (int i = 0; i < 10; i++) {
			if (serverMap.count(first_level_id - i)) {
				actualMap[first_level_id - i] = serverMap[first_level_id - i];
			}
			else {
				actualMap[first_level_id - i] = { nullptr, nullptr, -1, first_level_id - i };
			}
		}

		RTAGlobal::loadingUnknown = true;

		float offset = (float)list_entries_node->getChildrenCount() * cell_height;

		
		while (true) {
			break;

			RTAGlobal::LevelObject obj = { 0 };
			obj.cell = nullptr;
			obj.childIndex = -1;
			obj.level = nullptr;
			obj.level_id = 128;

			actualMap = { {128, obj} };

			obj.cell = nullptr;
			obj.childIndex = -1;
			obj.level = nullptr;
			obj.level_id = 121;

			actualMap[121] = obj;

			break;
		}

		for (auto& [id, obj] : actualMap) {
			if (obj.childIndex != -1) continue;

			targets.push_back(id);

			log::info("ANALYSYS: level {} is private/deleted", id);

			RTAGlobal::deletedLevelsSize++;

			LevelCell* cell = new LevelCell("a", 356.f, 90.f);
			if (!cell->init()) continue;
			cell->autorelease();
			cell->setID(fmt::format("{}", id));
			cell->setContentSize({ 356.f, 90.f });
			cell->loadFromLevel(nullptr);
			cell->m_backgroundLayer->setOpacity(140);

			obj.cell = cell;

			list_entries_node->addChild(obj.cell);

			obj.cell->setPositionY(offset);
			// obj.cell->setID(fmt::format("deleted{}", id));

			offset += 90.f;
		}

		int actual_height = 0.f;
		children = list_entries_node->getChildren();

		for (int i = 0; i < children->count(); i++) {
			CCObject* cellRawObject = children->objectAtIndex(i);
			LevelCell* levelObject = typeinfo_cast<LevelCell*>(cellRawObject);

			actual_height += levelObject->getContentHeight();
		}

		float szH = actual_height;

		ColumnLayout* cl = ColumnLayout::create();
		cl->setAutoScale(false);
		cl->setGap(0.f);
		cl->setAxisAlignment(AxisAlignment::Start);
		cl->setCrossAxisAlignment(AxisAlignment::Start);
		cl->setCrossAxisLineAlignment(AxisAlignment::Start);

		// list_entries_node->setLayout(cl);
		// list_entries_node->updateLayout();

		list_entries_node->setContentHeight(szH);

		RTAGlobal::loadingUnknown = false;

		m_fields->list_entries_node = list_entries_node;

		if (targets.size() == 0) return;

		std::string targets_str = fmt::format("{}", targets[0]);

		for (int i = 1; i < targets.size(); i++) {
			targets_str += fmt::format(",{}", targets[i]);
		}
		
		std::string postData = fmt::format("str={}&type=26&secret=Wmfd2893gb7", targets_str);
		
		std::string a = fmt::format("https://www.boomlings.com/database/getGJLevels21.php");

		web::WebRequest req = web::WebRequest();
		req.userAgent("");
		req.bodyString(postData);
		// Set a timeout for the request, in seconds
		req.timeout(std::chrono::seconds(5));
		auto task = req.post(a);

		m_fields->m_listener.bind([this](web::WebTask::Event* e) {
			if (web::WebResponse* value = e->getValue()) {
				if (!value->ok()) {
					log::error("response error 1");	

					return;
				}

				auto respString = value->string().unwrapOr("-1");

				if (respString == "-1") { // -1
					for (auto lobj : RTAGlobal::deletedLevels) {
						LevelCell* cell = typeinfo_cast<LevelCell*>(lobj.cell);

						if (!cell) {
							log::error("logic error 1");

							continue;
						}

						cell->loadFromLevel(nullptr);
					}

					return;
				}

				// 1:108514391:2:Wave Dance:5:1:6:251972024:8:0:9:0:10:0:12:10:13:22:14:0:17::43:0:25::18:0:19:0:42:0:45:1:3::15:3:30:0:31:0:37:0:38:0:39:10:46:1:47:2:35:0#251972024:1435Gunjan:29171654##9999:0:10#127875862cff610d0a37d0ec035798bb557e2c5d

				// respString = "1:108514391:2:Wave Dance:5:1:6:251972024:8:0:9:0:10:0:12:10:13:22:14:0:17::43:0:25::18:0:19:0:42:0:45:1:3::15:3:30:0:31:0:37:0:38:0:39:10:46:1:47:2:35:0#251972024:1435Gunjan:29171654##9999:0:10#127875862cff610d0a37d0ec035798bb557e2c5d";

				resolveLevelsFromString(respString);
			}
		});
		m_fields->m_listener.setFilter(task);
	}

	void resolveLevelsFromString(const std::string& data) {
		//// Took from https://github.com/SergeyMC9730/levelapipp/blob/master/GDServer_BoomlingsLike19.cpp
		//// And ported to Geode

		// create level array
		std::map<int, GJGameLevel*> vec1;

		// split array into level array and player array
		std::vector<std::string> vec2 = RTAGlobal::splitString(data.c_str(), '#');

		// get player list
		std::string plList = vec2[1];

		// get level list
		std::string lvlList = vec2[0];

		// create player map
		std::map<int, RTAGlobal::Account19 *> playerMap;

		// split player list into individual players
		std::vector<std::string> vec4array = RTAGlobal::splitString(plList.c_str(), '|');

		// split level list into individual levels
		std::vector<std::string> vec5levels = RTAGlobal::splitString(lvlList.c_str(), '|');

		int i = 0;

		std::vector<RTAGlobal::Account19 *> accounts;

		for (auto player_string : vec4array) {
			// split robtop array
			std::vector<std::string> player_data = RTAGlobal::splitString(player_string.c_str(), ':');

			if (player_data.size() >= 3) {
				// get strings for each array element;
				std::string userID_string = player_data[0];
				std::string accountID_string = player_data[2];
				std::string username = player_data[1];

				// parse userID
				int userID = atoi(userID_string.c_str());
				// parse accountID
				int accountID = atoi(accountID_string.c_str());

				RTAGlobal::Account19 *account = new RTAGlobal::Account19();

				account->accountID = accountID;
				account->userID = userID;
				account->username = username;

				playerMap[userID] = account;
				accounts.push_back(account);
			}
		}

#define SAFE_STOI(_INPUT, _OUTPUT) if (!_INPUT.empty()) { try { _OUTPUT = std::stoi(_INPUT); } catch (std::invalid_argument &e) { log::error("error while parsing {} : {}", _INPUT, e); } }

		for (auto &level_string : vec5levels) {
			// we'll parse level data manually instead of relaying on levelapi stuff

			auto vec_test = RTAGlobal::parseObjectData(level_string, ':');

			// parse level
			GJGameLevel* lvl = GJGameLevel::create();
			lvl->retain();

			SAFE_STOI(vec_test[1], lvl->m_levelID);
			lvl->m_levelName = vec_test[2];
			SAFE_STOI(vec_test[6], lvl->m_userID);
			SAFE_STOI(vec_test[10], lvl->m_downloads);
			SAFE_STOI(vec_test[12], lvl->m_audioTrack);
			SAFE_STOI(vec_test[5], lvl->m_levelVersion);
			SAFE_STOI(vec_test[13], lvl->m_gameVersion);
			SAFE_STOI(vec_test[14], lvl->m_likes);
			if (vec_test.count(9)) {
				lvl->m_difficulty = (GJDifficulty)(std::stoi(vec_test[9]));
			}
			if (vec_test.count(8)) {
				int userrated = std::stoi(vec_test[8]);
				if (userrated == 0) {
					// lvl->m_difficulty = GJDifficulty::NA;
				}
			}
			SAFE_STOI(vec_test[17], lvl->m_demon);
			SAFE_STOI(vec_test[18], lvl->m_stars);
			SAFE_STOI(vec_test[19], lvl->m_rateFeature);
			SAFE_STOI(vec_test[42], lvl->m_isEpic);
			SAFE_STOI(vec_test[45], lvl->m_objectCount);
			SAFE_STOI(vec_test[15], lvl->m_levelLength);
			SAFE_STOI(vec_test[30], lvl->m_originalLevel);
			SAFE_STOI(vec_test[31], lvl->m_twoPlayerMode);
			SAFE_STOI(vec_test[37], lvl->m_coins);
			SAFE_STOI(vec_test[38], lvl->m_coinsVerified);
			SAFE_STOI(vec_test[39], lvl->m_starsRequested);
			if (vec_test.count(35)) {
				lvl->m_songID = std::stoi(vec_test[35]);
			}
			if (vec_test.count(52)) {
				lvl->m_songIDs = vec_test[52];
			}
			if (vec_test.count(53)) {
				lvl->m_sfxIDs = vec_test[53];
			}

			// get account by user id
			RTAGlobal::Account19 *account = playerMap[lvl->m_userID];

			// set account values if it was found
			if (account != nullptr) {
				lvl->m_accountID = account->accountID;
				lvl->m_creatorName = account->username;
			}

			vec1[lvl->m_levelID] = lvl;

			int rsv_v = lvl->m_levelID;
		}

		// delete accounts
		for (auto account : accounts) delete account;

		for (RTAGlobal::LevelObject& obj : RTAGlobal::deletedLevels) {
			if (obj.cell != nullptr) {
				LevelCell* cell = typeinfo_cast<LevelCell*>(obj.cell);

				if (!vec1.count(obj.level_id)) {
					if (cell) {
						cell->loadFromLevel(nullptr);
					}

					continue;
				}

				if (cell) {
					cell->loadFromLevel(vec1[obj.level_id]);
				}
			}
		}

		CCLabelBMFont* list_title = typeinfo_cast<CCLabelBMFont*>(m_list->getChildByID("title"));

		std::string currentStr = list_title->getString();
		std::string new_str = fmt::format("{} ({} hidden)", currentStr, RTAGlobal::deletedLevels.size());

		list_title->setString(new_str.c_str(), true);
		list_title->setScale(0.675f);
	}
};