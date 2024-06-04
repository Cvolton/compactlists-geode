#include <Geode/Geode.hpp>
#include <Geode/modify/CustomListView.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>
#include <Geode/modify/LevelCell.hpp>

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

		return CustomListView::create(a, b, c, d, e, f, g);
	}
};

class $modify(LevelCell) {
	static void onModify(auto& self) {
        if (!self.setHookPriority("LevelCell::loadCustomLevelCell", 10000)) {
            log::warn("Failed to set LevelCell::loadCustomLevelCell hook priority, compact lists may not work properly");
        }
    }

	void moveNextToCoins(CCNode* node) {
		if(!node) return;

		std::array<const char*, 4> candidates = {
			"coin-icon-3",
			"coin-icon-2",
			"coin-icon-1",
			"song-name"
		};

		CCNode* finalist = nullptr;

		for(auto candidate : candidates) {
			finalist = m_mainLayer->getChildByID(candidate);
			
			if(finalist) break;
		}

		if(!finalist) return;

		node->setPositionX(finalist->getPositionX() + finalist->getScaledContentSize().width + 5 + node->getScaledContentSize().width / 2);
		node->setPositionY(finalist->getPositionY());
	}

	void moveNextToCreator(CCNode* node) {
		if(!node) return;

		std::array<const char*, 2> candidates = {
			"high-object-indicator",
			"copy-indicator"
		};

		CCNode* finalist = nullptr;

		for(auto candidate : candidates) {
			finalist = m_mainLayer->getChildByID(candidate);
			
			if(finalist) break;
		}

		if(finalist) {
			node->setPositionX(finalist->getPositionX() + finalist->getScaledContentSize().width + 5 + node->getScaledContentSize().width / 2);
			node->setPositionY(finalist->getPositionY());
		} else {
			auto levelName = m_mainLayer->getChildByID("level-name");
			if(!levelName) return;

			auto menu = m_mainLayer->getChildByID("main-menu");
			if(!menu) return;

			auto creatorName = menu->getChildByID("creator-name");
			if(!creatorName) return;

			node->setPositionX(levelName->getPositionX() + levelName->getScaledContentSize().width + 5 + creatorName->getScaledContentSize().width + 5 + node->getScaledContentSize().width / 2);
			node->setPositionY(levelName->getPositionY());
		}

	}

	void moveNextToView(CCNode* node) {
		if(!node) return;

		auto viewButton = m_mainMenu->getChildByID("view-button");
		if(!viewButton) return;

		/*node->setPositionX(viewButton->getPositionX() - viewButton->getScaledContentSize().width - 5 - node->getScaledContentSize().width / 2);
		node->setPositionY(viewButton->getPositionY());

		node->setPosition(m_mainLayer->convertToNodeSpace(m_mainMenu->convertToWorldSpace(node->getPosition())));*/
		node->setPosition({276, 25});
		//node->setPosition({276 - (node->getScaledContentSize().width / 2), 25});
	}

	/**
	 * Hooks
	*/

	void loadCustomLevelCell() {
		LevelCell::loadCustomLevelCell();

		if(m_compactView && m_level->m_unkInt == 0) {
			if(auto label = m_mainLayer->getChildByID("level-place")) {
				label->setVisible(false);

				for(auto child : CCArrayExt<CCNode*>(m_mainLayer->getChildren())) {
					if(child->getID() != "main-menu") {
						child->setPositionX(child->getPositionX() - 20);
					}
				}

				if(auto menu = m_mainLayer->getChildByID("main-menu")) {
					for(auto child : CCArrayExt<CCNode*>(menu->getChildren())) {
						if(child->getID() != "view-button") {
							child->setPositionX(child->getPositionX() - 20);
						}
					}
				}

				moveNextToView(m_mainLayer->getChildByID("completed-icon"));
				moveNextToView(m_mainLayer->getChildByID("percentage-label"));
			}
		}
	}
};

class $modify(CLLevelBrowserLayer, LevelBrowserLayer) {
	struct Fields {
		CCSprite* m_compactButton = nullptr;
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

			setButtonColor();

			menu->addChild(button);
			menu->updateLayout();
		}

		return true;
	}
};