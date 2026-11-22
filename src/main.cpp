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

class $modify(CLCustomListView, CustomListView) {
	struct Fields {
		bool m_ours = false;
	};

	static inline bool s_creatingOurs = false;

	void scrollToIndex(int index, bool clampToMaxY) {
        if (m_entries->count() == 1) return;

        float halfHeight = m_height * 0.5f;
        if (static_cast<float>(index - 1) * m_itemSeparation <= halfHeight) return;

        auto contentPos = m_tableView->m_contentLayer->getPosition();
        float targetY = (static_cast<float>(index + 1) * m_itemSeparation - halfHeight - m_itemSeparation * 0.5f)
                        + contentPos.y;

        if (clampToMaxY) {
            float maxY = m_tableView->getMaxY();
            if (targetY > maxY) {
                targetY = maxY;
            }
        }

        m_tableView->setContentOffset({contentPos.x, targetY}, false);
    };

	/**
	 * Hooks
	 */

	static CustomListView* create(cocos2d::CCArray* a, TableViewCellDelegate* b, float c, float d, int e, BoomListType f, float g) {
		bool ours = false;
		if(f == BoomListType::Level && Mod::get()->getSettingValue<bool>("enable-compact-lists")) {
			f = BoomListType::Level4;
			ours = true;
		}
		if(f == BoomListType::Score && Mod::get()->getSettingValue<bool>("enable-compact-scores")) {
			f = BoomListType::CompactUser;
			ours = true;
		}

		s_creatingOurs = ours;
		auto ret = CustomListView::create(a, b, c, d, e, f, g);
		static_cast<CLCustomListView*>(ret)->m_fields->m_ours = ours;
		s_creatingOurs = false;

		return ret;
	}

	void setupList(float yOffset) {
		CustomListView::setupList(yOffset);

		if(s_creatingOurs && m_type == BoomListType::CompactUser) {
			int foundIndex = -1;
            for (unsigned int i = 0; i < m_entries->count(); ++i) {
                auto score = static_cast<GJUserScore*>(m_entries->objectAtIndex(i));
                if (score->isCurrentUser()) {
                    foundIndex = static_cast<int>(i);
                    break;
                }
            }
			
            if (foundIndex != -1) {
                scrollToIndex(foundIndex, false);
            }
		}
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

		if(score->m_playerRank > 0) {

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

		}

		if(auto menu = m_mainLayer->getChildByID("stats-menu")) {
			moveToTheEnd(menu->getChildByID("creator-points-label"));
			moveToTheEnd(menu->getChildByID("creator-points-icon"));

			menu->setContentSize({270.f, 60});
			menu->updateLayout();
		}

		if(score->isCurrentUser()) {
			m_backgroundLayer->setColor({230, 150, 10});
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
		switch(m_searchObject->m_searchType) {
			case SearchType::Users:
				Mod::get()->setSettingValue("enable-compact-scores", !Mod::get()->getSettingValue<bool>("enable-compact-scores"));
				break;
			default:
				Mod::get()->setSettingValue("enable-compact-lists", !Mod::get()->getSettingValue<bool>("enable-compact-lists"));
				break;
		}

		loadPage(m_searchObject);
	}

	bool compactListEnabled() {
		switch(m_searchObject->m_searchType) {
			case SearchType::Users:
				return Mod::get()->getSettingValue<bool>("enable-compact-scores");
			default:
				return Mod::get()->getSettingValue<bool>("enable-compact-lists");
		}
	}

	void setButtonColor() {
		if(m_fields->m_compactButton) {
			m_fields->m_compactButton->setColor(compactListEnabled() ? ccc3(255, 255, 255) : ccc3(125, 125, 125));
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