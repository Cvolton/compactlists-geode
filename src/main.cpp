#include <Geode/Geode.hpp>
#include <Geode/modify/CustomListView.hpp>
#include <Geode/modify/LeaderboardsLayer.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>
#include <Geode/modify/LevelListLayer.hpp>
#include <Geode/modify/LevelCell.hpp>
#include <Geode/modify/GJCompactUserCell.hpp>

using namespace geode::prelude;

/*class $modify(LevelBrowserLayer) {
	void setupLevelBrowser(CCArray* array) {
		m_listHeight = 40.f;

		log::info("{} test", m_test2);


		LevelBrowserLayer::setupLevelBrowser(array);
	} 
};*/

class $modify(CustomListView) {
	static CustomListView* create(cocos2d::CCArray* a, TableViewCellDelegate* b, float c, float d, int e, BoomListType f, float g) {
		if(f == BoomListType::Level && Mod::get()->getSettingValue<bool>("enable-compact-lists")) f = BoomListType::Level4;
		if(f == BoomListType::Score && Mod::get()->getSettingValue<bool>("enable-compact-scores")) f = BoomListType::CompactUser;

		return CustomListView::create(a, b, c, d, e, f, g);
	}
};

class $modify(LevelCell) {
	static void onModify(auto& self) {
        if (!self.setHookPriority("LevelCell::loadCustomLevelCell", Priority::LatePre)) {
            log::warn("Failed to set LevelCell::loadCustomLevelCell hook priority, compact lists may not work properly");
        }
    }

	/**
	 * Hooks
	*/

	void loadCustomLevelCell() {
		m_compactNoIndex = m_level->m_listPosition == 0 && !this->getUserFlag("skip-adjustment"_spr);
		LevelCell::loadCustomLevelCell();
	}
};

class $modify(GJCompactUserCell) {
	static void onModify(auto& self) {
        if (!self.setHookPriority("GJCompactUserCell::loadFromScore", Priority::EarlyPost)) {
            log::warn("Failed to set GJCompactUserCell::loadFromScore hook priority, compact lists may not work properly");
        }
    }

	void addStat(const char* id, const char* texture, int value) {
		if(auto menu = m_mainLayer->getChildByID("stats-menu")) {
			auto label = Label::create(GameToolbox::intToShortString(value, true), "bigFont.fnt");

			auto options = AxisLayoutOptions::create()
				->setScaleLimits(.0f, 1.f)
				->setRelativeScale(.4f);

			label->setLayoutOptions(options);
			label->setID(fmt::format("{}-label"_spr, id));
			menu->addChild(label);

			auto icon = CCSprite::createWithSpriteFrameName(texture);
			auto iconOptions = AxisLayoutOptions::create()
				->setScaleLimits(.0f, 1.f)
				->setRelativeScale(.5f)
				->setNextGap(8.f);
			icon->setLayoutOptions(iconOptions);
			menu->addChild(icon);

			menu->updateLayout();
		}

	}

	void moveToTheEnd(Ref<CCNode> node) {
		if(!node) return;

		auto parent = node->getParent();
		if(!parent) return;

		node->removeFromParent();
		parent->addChild(node);
	}

	/**
	 * Hooks
	*/

	void loadFromScore(GJUserScore* score) {
		GJCompactUserCell::loadFromScore(score);

		addStat("coins", "GJ_coinsIcon_001.png", score->m_secretCoins);
		addStat("user-coins", "GJ_coinsIcon2_001.png", score->m_userCoins);
		// nobody care about diamond imo

		if(auto icon = m_mainLayer->getChildByID("player-icon")) {
			icon->setPositionY(icon->getPositionY() + 5.75f);
			icon->setScale(.7f);

			auto rankLabel = Label::create(fmt::to_string(score->m_playerRank), "goldFont.fnt");
			rankLabel->setID("rank-label"_spr);

			rankLabel->setPosition(icon->getPosition());
			rankLabel->setPositionY(rankLabel->getPositionY() - 17.f);
			rankLabel->setScale(.45f);

			m_mainLayer->addChild(rankLabel);
		}

		if(auto menu = m_mainLayer->getChildByID("stats-menu")) {
			moveToTheEnd(menu->getChildByID("creator-points-label"));
			moveToTheEnd(menu->getChildByID("creator-points-icon"));

			menu->setContentSize({270.f, 60});
			menu->updateLayout();
		}
	}
};

class $modify(CLLeaderboardsLayer, LeaderboardsLayer) {
	struct Fields {
		Ref<CCSprite> m_compactButton = nullptr;
	};

	void onCompactListToggle(CCObject* sender) {
		Mod::get()->setSettingValue("enable-compact-scores", !Mod::get()->getSettingValue<bool>("enable-compact-scores"));

		auto scene = LeaderboardsLayer::scene(m_type, m_stat);
        CCDirector::sharedDirector()->replaceScene(scene);
	}

	void setButtonColor() {
		if(m_fields->m_compactButton) {
			m_fields->m_compactButton->setColor(Mod::get()->getSettingValue<bool>("enable-compact-scores") ? ccc3(255, 255, 255) : ccc3(125, 125, 125));
		}
	}

	/**
	 * Hooks
	*/
	/*void onEnter() {
		LeaderboardsLayer::onEnter();

		setButtonColor();
	}*/

	void setupLevelBrowser(CCArray* array) {
		LeaderboardsLayer::setupLevelBrowser(array);

		setButtonColor();
	}

	bool init(LeaderboardType type, LeaderboardStat stat) {
		if(!LeaderboardsLayer::init(type, stat)) return false;
		if(!Mod::get()->getSettingValue<bool>("show-toggle")) return true;

		if(auto menu = getChildByID("info-menu")) {
			auto button = CCMenuItemSpriteExtra::create(
				m_fields->m_compactButton = CCSprite::createWithSpriteFrameName("GJ_smallModeIcon_001.png"),
				this,
				menu_selector(CLLeaderboardsLayer::onCompactListToggle)
			);
			button->setZOrder(-1);
			button->setLayoutOptions(
				AxisLayoutOptions::create()
					->setRelativeScale(.95f)
			);
			button->setID("compact-button"_spr);

			setButtonColor();

			menu->addChild(button);
			menu->updateLayout();
		}

		return true;
	}
};

class $modify(CLLevelBrowserLayer, LevelBrowserLayer) {
	struct Fields {
		Ref<CCSprite> m_compactButton = nullptr;
	};

	void onCompactListToggle(CCObject* sender) {
		Mod::get()->setSettingValue("enable-compact-lists", !Mod::get()->getSettingValue<bool>("enable-compact-lists"));
		loadPage(m_searchObject);
	}

	void setButtonColor() {
		if(m_fields->m_compactButton) {
			m_fields->m_compactButton->setColor(Mod::get()->getSettingValue<bool>("enable-compact-lists") ? ccc3(255, 255, 255) : ccc3(125, 125, 125));
		}
	}

	/**
	 * Hooks
	*/
	void onEnter() {
		LevelBrowserLayer::onEnter();

		setButtonColor();
	}

	void setupLevelBrowser(CCArray* array) {
		LevelBrowserLayer::setupLevelBrowser(array);

		setButtonColor();
	}

	bool init(GJSearchObject* obj) {
		if(!LevelBrowserLayer::init(obj)) return false;
		if(!Mod::get()->getSettingValue<bool>("show-toggle")) return true;
		if(m_searchObject->m_searchMode != 0) return true;

		if(auto menu = getChildByID("info-menu")) {
			auto button = CCMenuItemSpriteExtra::create(
				m_fields->m_compactButton = CCSprite::createWithSpriteFrameName("GJ_smallModeIcon_001.png"),
				this,
				menu_selector(CLLevelBrowserLayer::onCompactListToggle)
			);
			button->setZOrder(-1);
			button->setLayoutOptions(
				AxisLayoutOptions::create()
					->setRelativeScale(.95f)
			);
			button->setID("compact-button"_spr);

			setButtonColor();

			menu->addChild(button);
			menu->updateLayout();
		}

		return true;
	}
};

class $modify(LevelListLayer) {
	void onBack(CCObject* sender) {
		for(auto level : CCArrayExt<GJGameLevel*>(m_list->m_listView->m_entries)) {
			level->m_listPosition = 0;
		}

		LevelListLayer::onBack(sender);
	}
};