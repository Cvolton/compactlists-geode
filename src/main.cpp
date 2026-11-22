#include <Geode/Geode.hpp>
#include <Geode/modify/CustomListView.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>
#include <Geode/modify/LevelListLayer.hpp>
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
        if (!self.setHookPriority("LevelCell::loadCustomLevelCell", Priority::EarlyPost)) {
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