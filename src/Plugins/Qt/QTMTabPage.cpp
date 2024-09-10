
/******************************************************************************
 * MODULE     : QTMTabPage.cpp
 * DESCRIPTION: QT Texmacs tab page classes
 * COPYRIGHT  : (C) 2024 Zhenjun Guo
 *******************************************************************************
 * This software falls under the GNU general public license version 3 or later.
 * It comes WITHOUT ANY WARRANTY WHATSOEVER. For details, see the file LICENSE
 * in the root directory or <http://www.gnu.org/licenses/gpl-3.0.html>.
 ******************************************************************************/

#include "QTMTabPage.hpp"

// The minimum width of a single tab page (in pixels).
#define MIN_TAB_PAGE_WIDTH 150

/**
 * What is g_mostRecentlyClosedTab used for? When we close an ACTIVE(!) tab
 * (let's denote it as T), the tab bar is refreshed twice, meaning that
 * QTMTabPageContainer::replaceTabPages is called twice. Specifically:
 *
 * -- During the first call, tab T has not yet been deleted, so T is still
 *    visible, although it is no longer in the active state.
 * -- During the second call, tab T has been deleted, and at this point, T is no
 *    longer visible.
 *
 * As a result, what the user observes is that when they close an ACTIVE tab, it
 * does not disappear immediately. Therefore, we need it to remember which tab
 * was most recently closed and avoid displaying it during the first update.
 */
url g_mostRecentlyClosedTab;

/******************************************************************************
 * QTMTabPage
 ******************************************************************************/

QTMTabPage::QTMTabPage (url p_url, QAction* p_title, QAction* p_closeBtn,
                        bool p_isActive)
    : m_bufferUrl (p_url) {
  p_title->setCheckable (p_isActive);
  p_title->setChecked (p_isActive);
  setDefaultAction (p_title);
  setFocusPolicy (Qt::NoFocus); // don't steal focus from the editor (1)

  m_closeBtn= new QToolButton (this);
  m_closeBtn->setObjectName ("closeBtn");
  m_closeBtn->setDefaultAction (p_closeBtn);
  m_closeBtn->setFixedSize (20, 20); // position will be updated in resizeEvent
  m_closeBtn->setFocusPolicy (
      Qt::NoFocus); // don't steal focus from the editor (2) (both are needed)
  connect (m_closeBtn, &QToolButton::clicked, this,
           [=] () { g_mostRecentlyClosedTab= m_bufferUrl; });

  setupStyle ();
}

/* We can't align the text to the left of the button by QSS or other methods,
 * so for now we achieve it by overriding the paintEvent. */
void
QTMTabPage::paintEvent (QPaintEvent*) {
  QStylePainter          p (this);
  QStyleOptionToolButton opt;
  initStyleOption (&opt);
  opt.text= "";                                      // don't draw the text now
  p.drawComplexControl (QStyle::CC_ToolButton, opt); // base method

  // draw the text now
  QFontMetrics fm (opt.fontMetrics);
  QRect        rect= fm.boundingRect (opt.rect, Qt::AlignVCenter, text ());
  rect.moveLeft (10);
  p.drawItemText (rect, Qt::AlignLeft, palette (), isEnabled (), text (),
                  QPalette::ButtonText);
}

void
QTMTabPage::resizeEvent (QResizeEvent* e) {
  int w= m_closeBtn->width ();
  int h= m_closeBtn->height ();
  int x= e->size ().width () - w - 8;
  int y= e->size ().height () / 2 - h / 2;

  m_closeBtn->setGeometry (x, y, w, h);
}

void
QTMTabPage::setupStyle () {
  QString qss, bg_color, bg_color_hover, border_color, border_color_top;

#ifdef Q_OS_WINDOWS
  bg_color        = "#C7C8C9";
  bg_color_hover  = "#F5F5F5";
  border_color    = "#A6A6A6";
  border_color_top= "#3DAEE9";
#endif

#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
  bg_color        = "#C7C8C9";
  bg_color_hover  = "#EFF0F1";
  border_color    = "#A6A6A6";
  border_color_top= "#3DAEE9";
#endif

  qss+= QString ("QTMTabPage{ padding: 0 26px; border-radius: 0px; "
                 "background-color: %1; }")
            .arg (bg_color);
  qss+= QString ("QTMTabPage:hover{ background-color: %1; }")
            .arg (bg_color_hover);
  qss+= QString (
            "QTMTabPage:checked{ background-color: %1; border-top: 3px solid "
            "%2; border-left: 1px solid %3; border-right: 1px solid %4; }")
            .arg (bg_color_hover, border_color_top, border_color, border_color);

  qss+= "#closeBtn{ background-color: transparent; border-radius: 0px; "
        "padding: 0px; }";
  qss+= "#closeBtn:hover{ border-radius: 10px; color: #ffffff; "
        "background-color: #E49AA2; }";

  setStyleSheet (qss);
}

/******************************************************************************
 * QTMTabPageContainer
 ******************************************************************************/

QTMTabPageContainer::QTMTabPageContainer (QWidget* p_parent)
    : QWidget (p_parent) {
  setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Preferred);
}

QTMTabPageContainer::~QTMTabPageContainer () { removeAllTabPages (); }

void
QTMTabPageContainer::replaceTabPages (QList<QAction*>* p_src) {
  removeAllTabPages ();    // remove  old tabs
  extractTabPages (p_src); // extract new tabs

  const int windowWidth= this->width ();
  int       rowCount   = 0;
  int       accumWidth = 0;
  int       accumTab   = 0;

  for (int i= 0; i < m_tabPageList.size (); ++i) {
    QTMTabPage* tab= m_tabPageList[i];
    if (g_mostRecentlyClosedTab == tab->m_bufferUrl) {
      // this tab has just been closed, don't display it
      tab->hide ();
      continue;
    }

    QSize tabSize = tab->minimumSizeHint ();
    int   tabWidth= max (MIN_TAB_PAGE_WIDTH, tabSize.width ());
    if (accumWidth + tabWidth >= windowWidth) {
      rowCount+= 1;
      accumWidth= 0;
      accumTab  = 0;
    }
    tab->setGeometry (accumWidth, rowCount * m_rowHeight, tabWidth,
                      m_rowHeight);
    accumWidth+= tabWidth;
    accumTab+= 1;
  }

  g_mostRecentlyClosedTab= url (); // clear memory
  adjustHeight (rowCount);
}

void
QTMTabPageContainer::removeAllTabPages () {
  for (int i= 0; i < m_tabPageList.size (); ++i) {
    delete m_tabPageList[i];
  }
  m_tabPageList.clear ();
}

void
QTMTabPageContainer::extractTabPages (QList<QAction*>* p_src) {
  if (!p_src) return;
  for (int i= 0; i < p_src->size (); ++i) {
    // see the definition of QTMTabPageAction why we're using it
    QTMTabPageAction* carrier= qobject_cast<QTMTabPageAction*> ((*p_src)[i]);
    ASSERT (carrier, "QTMTabPageAction expected")

    QTMTabPage* tab= qobject_cast<QTMTabPage*> (carrier->m_widget);
    if (tab) {
      tab->setParent (this);
      m_tabPageList.append (tab);
    }
    else {
      delete carrier->m_widget; // we don't use it so we should delete it
    }

    delete carrier; // we don't need it anymore
  }
}

void
QTMTabPageContainer::adjustHeight (int p_rowCount) {
  int h= m_rowHeight * (p_rowCount + 1);
  // parentWidget's resizeEvent() will resize me
  parentWidget ()->setFixedHeight (h + 2);
}

/******************************************************************************
 * QTMTabPageBar
 ******************************************************************************/

QTMTabPageBar::QTMTabPageBar (const QString& p_title, QWidget* p_parent)
    : QToolBar (p_title, p_parent) {
  m_container= new QTMTabPageContainer (this);
}

void
QTMTabPageBar::replaceTabPages (QList<QAction*>* p_src) {
  setUpdatesEnabled (false);
  bool visible= this->isVisible ();
  if (visible) hide (); // TRICK: to avoid flicker of the dest widget

  m_container->replaceTabPages (p_src);

  if (visible) show (); // TRICK: see above
  setUpdatesEnabled (true);
}

void
QTMTabPageBar::resizeEvent (QResizeEvent* e) {
  QSize size= e->size ();
  // Reserve 7px space on the left for the handle of QToolbar
  m_container->setGeometry (7, 0, size.width (), size.height () - 2);
}