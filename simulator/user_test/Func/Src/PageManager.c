/* SPDX-License-Identifier: GPL-3.0-only
 * Screen ownership is explicit: deactivate once, load new screen, delete old.
 */
#include "../Inc/PageManager.h"
#include "../../GUI_App/Screens/Inc/ui_HomePage.h"
#include "../../GUI_App/Screens/Inc/ui_MenuPage.h"
PageStack_t PageStack;
static void show(Page_t *page,lv_obj_t *old)
{
    page->init();
    lv_scr_load(*page->page_obj);
    if(old && old!=*page->page_obj) lv_obj_del(old);
}
Page_t *Page_Get_NowPage(void)
{
    return PageStack.top ? PageStack.pages[PageStack.top-1] : NULL;
}
void Page_Load(Page_t *page)
{
    if(!page || PageStack.top>=MAX_DEPTH || Page_Get_NowPage()==page) return;
    lv_obj_t *old=lv_scr_act();
    Page_t *active=Page_Get_NowPage();
    if(active) active->deinit();
    PageStack.pages[PageStack.top++]=page;
    show(page,old);
}
void Page_Back(void)
{
    Page_t *active=Page_Get_NowPage();
    if(!active) return;
    lv_obj_t *old=lv_scr_act();
    active->deinit();
    if(PageStack.top>1) PageStack.top--;
    else {
        PageStack.pages[0]=&Page_Home;
        PageStack.pages[1]=&Page_Menu;
        PageStack.top=2;
    }
    show(Page_Get_NowPage(),old);
}
void Page_Back_Bottom(void)
{
    if(PageStack.top<=1) return;
    lv_obj_t *old=lv_scr_act();
    Page_Get_NowPage()->deinit();
    PageStack.top=1;
    show(Page_Get_NowPage(),old);
}
void Pages_init(void)
{
    lv_obj_t *old=lv_scr_act();
    PageStack.top=1;
    PageStack.pages[0]=&Page_Home;
    show(&Page_Home,old);
}
