/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 *
 * gGraphView Header
 *
 * Copyright (c) 2011-2014 Mark Watkins <jedimark@users.sourceforge.net>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of the Linux
 * distribution for more details. */

#ifndef GGRAPHVIEW_H
#define GGRAPHVIEW_H

#include <QScrollBar>
#include <QResizeEvent>
#include <QThread>
#include <QMutex>
#include <QSemaphore>
#include <QWaitCondition>
#include <QPixmap>
#include <QRect>
#include <QPixmapCache>

#ifndef BROKEN_OPENGL_BUILD
#include <QGLWidget>
#endif

#include <Graphs/gGraph.h>
#include <Graphs/glcommon.h>
#include <SleepLib/day.h>

enum FlagType { FT_Bar, FT_Dot, FT_Span };

class gGraphView;

const int textque_max = 512;

/*! \struct TextQue
    \brief Holds a single item of text for the drawing queue
    */
struct TextQue {
    TextQue() {
    }
    TextQue(short x, short y, float angle, QString text, QColor color, QFont * font, bool antialias):
    x(x), y(y), angle(angle), text(text), color(color), font(font), antialias(antialias)
    {
    }
    TextQue(const TextQue & copy) {
        x=copy.x;
        y=copy.y;
        text=copy.text;
        angle=copy.angle;
        color=copy.color;
        font=copy.font;
        antialias=copy.antialias;
    }

    //! \variable contains the x axis screen position to draw the text
    short x;
    //! \variable contains the y axis screen position to draw the text
    short y;
    //! \variable the angle in degrees for drawing rotated text
    float angle;
    //! \variable the actual text to draw
    QString text;
    //! \variable the color the text will be drawn in
    QColor color;
    //! \variable a pointer to the QFont to use to draw this text
    QFont *font;
    //! \variable whether to use antialiasing to draw this text
    bool antialias;
};

/*! \class MyScrollBar
    \brief An custom scrollbar to interface with gGraphWindow
    */
class MyScrollBar : public QScrollBar
{
  public:
    MyScrollBar(QWidget *parent = nullptr)
        : QScrollBar(parent)
    { }

    void SendWheelEvent(QWheelEvent *e) {
        wheelEvent(e);
    }
};

/*! \class gThread
    \brief Part of the Threaded drawing code

    This is currently broken, as Qt didn't behave anyway, and it offered no performance improvement drawing-wise
    */
class gThread : public QThread
{
  public:
    gThread(gGraphView *g);
    ~gThread();

    //! \brief Start thread process
    void run();

    //! \brief Kill thread process
    void die() { m_running = false; }
    QMutex mutex;

  protected:
    gGraphView *graphview;
    volatile bool m_running;
};

/*! \class gToolTip
    \brief Popup Tooltip to display information over the OpenGL graphs
    */
class gToolTip : public QObject
{
    Q_OBJECT
  public:
    //! \brief Initializes the ToolTip object, and connects the timeout to the gGraphView object
    gToolTip(gGraphView *graphview);
    virtual ~gToolTip();

    /*! \fn virtual void display(QString text, int x, int y, int timeout=2000);
        \brief Set the tooltips display message, position, and timeout value
        */
    virtual void display(QString text, int x, int y, int timeout = 0);

    //! \brief Draw the tooltip
    virtual void paint(QPainter &paint); //actually paints it.

    //! \brief Close the tooltip early.
    void cancel();

    //! \brief Returns true if the tooltip is currently visible
    bool visible() { return m_visible; }

  protected:
    gGraphView *m_graphview;
    QTimer *timer;
    QPoint m_pos;
    int tw, th;
    QString m_text;
    bool m_visible;
    int m_spacer;
    QImage m_image;
    bool m_invalidate;

  protected slots:

    //! \brief Timeout to hide tooltip, and redraw without it.
    void timerDone();
};

struct SelectionHistoryItem {
    SelectionHistoryItem() {
        minx=maxx=0;
    }
    SelectionHistoryItem(quint64 m1, quint64 m2) {
        minx=m1;
        maxx=m2;
    }

    SelectionHistoryItem(const SelectionHistoryItem & copy) {
        minx = copy.minx;
        maxx = copy.maxx;
    }
    quint64 minx;
    quint64 maxx;
};

/*! \class gGraphView
    \brief Main OpenGL Graph Area, derived from QGLWidget

    This widget contains a list of graphs, and provides the means to display them, scroll via an attached
    scrollbar, change the order around, and resize individual graphs.

    It replaced QScrollArea and multiple QGLWidgets graphs, and a very buggy QSplitter.

    It led to quite a performance increase over the old Qt method.

    */
class gGraphView
#ifdef BROKEN_OPENGL_BUILD
        :public QWidget
#else
        :public QGLWidget
#endif
{
    Q_OBJECT
  public:
    /*! \fn explicit gGraphView(QWidget *parent = 0,gGraphView * shared=0);
        \brief Constructs a new gGraphView object (main graph area)
        \param QWidget * parent
        \param gGraphView * shared

        The shared parameter allows for OpenGL context sharing.
        But this must not be shared with Printers snapshot gGraphView object,
        or it will lead to display/font corruption
        */
    explicit gGraphView(QWidget *parent = 0, gGraphView *shared = 0);
    virtual ~gGraphView();
    virtual void closeEvent(QCloseEvent * event);

    //! \brief Add gGraph g to this gGraphView, in the requested graph-linkage group
    void addGraph(gGraph *g, short group = 0);

    //! \brief The width of the vertical text Title area for all graphs
    static const int titleWidth = 30;

    //! \brief The splitter is drawn inside this gap
    static const int graphSpacer = 4;

    bool contains(QString name) {
        for (int i=0; i<m_graphs.size(); ++i) {
            if (m_graphs[i]->name() == name) return true;
        }
        return false;
    }
    gGraph *operator [](QString name) {
        for (int i=0; i<m_graphs.size(); ++i) {
            if (m_graphs[i]->name() == name) return m_graphs[i];
        }
        return nullptr;
    }

    //! \brief Finds the top pixel position of the supplied graph
    float findTop(gGraph *graph);

    //! \brief Returns the scaleY value, which is used when laying out less graphs than fit on the screen.
    float scaleY() { return m_scaleY; }

    //! \brief Sets the scaleY value, which is used when laying out less graphs than fit on the screen.
    void setScaleY(float sy) { m_scaleY = sy; }

    //! \brief Returns the current selected time range
    void GetXBounds(qint64 &st, qint64 &et);

    //! \brief Returns the maximum time range bounds
    void GetRXBounds(qint64 &st, qint64 &et);

    //! \brief Resets the time range to default for this day. Refreshing the display if refresh==true.
    void ResetBounds(bool refresh = true);

    //! \brief Supplies time range to all graph objects in linked group, refreshing if requested
    void SetXBounds(qint64 minx, qint64 maxx, short group = 0, bool refresh = true);

    //! \brief Saves the current graph order, heights, min & Max Y values to disk
    void SaveSettings(QString title);

    //! \brief Loads the current graph order, heights, min & max Y values from disk
    bool LoadSettings(QString title);

    //! \brief Returns the graph object matching the supplied name, nullptr if it does not exist.
    gGraph *findGraph(QString name);

    //! \brief Returns the graph object matching the graph title, nullptr if it does not exist.
    gGraph *findGraphTitle(QString title);

    //! \brief Returns true if control key is down during select operation
    inline bool metaSelect() const { return m_metaselect; }

    //! \brief Returns true if currently selecting data with mouse
    inline bool selectionInProgress() const { return m_button_down; }

    inline float printScaleX() const { return print_scaleX; }
    inline float printScaleY() const { return print_scaleY; }
    inline void setPrintScaleX(float x) { print_scaleX = x; }
    inline void setPrintScaleY(float y) { print_scaleY = y; }

    void saveHistory() {
        history.push_front(SelectionHistoryItem(m_minx, m_maxx));
        if (history.size() > max_history) {
            history.pop_back();
        }
    }

    //! \brief Returns true if all Graph objects contain NO day data. ie, graph area is completely empty.
    bool isEmpty();

    //! \brief Tell all graphs to deslect any highlighted areas
    void deselect();

    QPoint pointClicked() const { return m_point_clicked; }
    void setPointClicked(QPoint p) { m_point_clicked = p; }

    //! \brief Set a redraw timer for ms milliseconds, clearing any previous redraw timer.
    void timedRedraw(int ms);

    gGraph *m_selected_graph;
    gToolTip *m_tooltip;
    QTimer *timer;

    //! \brief Updates the current selection time in the statusbar area
    void updateSelectionTime();

    //! \brief Add the Text information to the Text Drawing Queue (called by gGraphs renderText method)
    void AddTextQue(const QString &text, short x, short y, float angle = 0.0,
                    QColor color = Qt::black, QFont *font = defaultfont, bool antialias = true);

//    //! \brief Draw all Text in the text drawing queue
//    void DrawTextQue();

    //! \brief Draw all text components using QPainter object painter
    void DrawTextQue(QPainter &painter);

    //! \brief Returns number of graphs contained (whether they are visible or not)
    int size() const { return m_graphs.size(); }

    //! \brief Return individual graph by index value
    gGraph *operator[](int i) { return m_graphs[i]; }

    //! \brief Returns the custom scrollbar object linked to this gGraphArea
    MyScrollBar *scrollBar() const { return m_scrollbar; }

    //! \brief Sets the custom scrollbar object linked to this gGraphArea
    void setScrollBar(MyScrollBar *sb);

    //! \brief Calculates the correct scrollbar parameters for all visible, non empty graphs.
    void updateScrollBar();

    //! \brief Called on resize, fits graphs when too few to show, by scaling to fit screen size. Calls updateScrollBar()
    void updateScale();         // update scale & Scrollbar

    //! \brief Resets all contained graphs to have a uniform height.
    void resetLayout();

    //! \brief Returns a count of all visible, non-empty Graphs.
    int visibleGraphs();

    //! \brief Returns the horizontal travel of the mouse, for use in Mouse Handling code.
    int horizTravel() const { return m_horiz_travel; }

    //! \brief Sets the message displayed when there are no graphs to draw
    void setEmptyText(QString s) { m_emptytext = s; }

    inline const float &devicePixelRatio() { return m_dpr; }
    void setDevicePixelRatio(float dpr) { m_dpr = dpr; }

#ifdef ENABLE_THREADED_DRAWING
    QMutex text_mutex;
    QMutex gl_mutex;
    QSemaphore *masterlock;
    bool useThreads() { return m_idealthreads > 1; }
    QVector<gThread *> m_threads;
    int m_idealthreads;
    QMutex dl_mutex;
#endif

    //! \brief Sends day object to be distributed to all Graphs Layers objects
    void setDay(Day *day);

    //! \brief pops a graph off the list for multithreaded drawing code
    gGraph *popGraph(); // exposed for multithreaded drawing

    //! \brief Hides the splitter, used in report printing code
    void hideSplitter() { m_showsplitter = false; }

    //! \brief Re-enabled the in-between graph splitters.
    void showSplitter() { m_showsplitter = true; }

    //! \brief Trash all graph objects listed (without destroying Graph contents)
    void trashGraphs();

    //! \brief Enable or disable the Text Pixmap Caching system preference overide
    void setUsePixmapCache(bool b) { use_pixmap_cache = b; }

    //! \brief Return whether or not the Pixmap Cache for text rendering is being used.
    bool usePixmapCache();

    //! \brief Graph drawing routines, returns true if there weren't any graphs to draw
    bool renderGraphs(QPainter &painter);

    //! \brief Used internally by graph mousehandler to set modifier state
    void setMetaSelect(bool b) { m_metaselect = b; }

    //! \brief The current time the mouse pointer is hovering over
    inline qint64 currentTime() { return m_currenttime; }

    inline void setCurrentTime(qint64 time) { m_currenttime = time; }

    inline QPoint currentMousePos() const { return m_mouse; }

    void dumpInfo();

    // for profiling purposes, a count of lines drawn in a single frame
    int lines_drawn_this_frame;
    int quads_drawn_this_frame;
    int strings_drawn_this_frame;
    int strings_cached_this_frame;

    QList<SelectionHistoryItem> history;

  protected:

    virtual void leaveEvent (QEvent * event);

    //! \brief The heart of the drawing code
#ifdef BROKEN_OPENGL_BUILD
    virtual void paintEvent(QPaintEvent *);
#else
    virtual void paintGL();
#endif
    //! \brief Calculates the sum of all graph heights
    float totalHeight();

    //! \brief Calculates the sum of all graph heights, taking scaling into consideration
    float scaleHeight();

    //! \brief Update the OpenGL area when the screen is resized
    virtual void resizeEvent(QResizeEvent *);

    //! \brief Set the Vertical offset (used in scrolling)
    void setOffsetY(int offsetY);

    //! \brief Set the Horizontal offset (not used yet)
    void setOffsetX(int offsetX);

    //! \brief Mouse Moved somewhere in main gGraphArea, propagates to the individual graphs
    virtual void mouseMoveEvent(QMouseEvent *event);
    //! \brief Mouse Button Press Event somewhere in main gGraphArea, propagates to the individual graphs
    virtual void mousePressEvent(QMouseEvent *event);
    //! \brief Mouse Button Release Event somewhere in main gGraphArea, propagates to the individual graphs
    virtual void mouseReleaseEvent(QMouseEvent *event);
    //! \brief Mouse Button Double Click Event somewhere in main gGraphArea, propagates to the individual graphs
    virtual void mouseDoubleClickEvent(QMouseEvent *event);
    //! \brief Mouse Wheel Event somewhere in main gGraphArea, propagates to the individual graphs
    virtual void wheelEvent(QWheelEvent *event);
    //! \brief Keyboard event while main gGraphArea has focus.
    virtual void keyPressEvent(QKeyEvent *event);
    //! \brief Keyboard event while main gGraphArea has focus.
    virtual void keyReleaseEvent(QKeyEvent *event);

    //! \brief Add Graph to drawing queue, mainly for the benefit of multithreaded drawing code
    void queGraph(gGraph *, int originX, int originY, int width, int height);

    Day *m_day;

    //! \brief the list of graphs to draw this frame
    QList<gGraph *> m_drawlist;

    //! \brief Linked graph object containing shared GL Context (in this app, daily view's gGraphView)
    gGraphView *m_shared;

    //! \brief List of all graphs contained in this area
    QVector<gGraph *> m_graphs;

    //! \brief List of all graphs contained, indexed by title
    QHash<QString, gGraph *> m_graphsbyname;

    //! \variable Vertical scroll offset (adjusted when scrollbar gets moved)
    int m_offsetY;
    //! \variable Horizontal scroll offset (unused, but can be made to work if necessary)
    int m_offsetX;
    //! \variable Scale used to enlarge graphs when less graphs than can fit on screen.
    float m_scaleY;
    float m_dpr;

    bool m_sizer_dragging;
    int m_sizer_index;

    bool m_button_down;
    QPoint m_point_clicked;
    QPoint m_point_released;
    bool m_metaselect;

    QPoint m_sizer_point;
    int m_horiz_travel;

    MyScrollBar *m_scrollbar;
    QTimer *redrawtimer;

    bool m_graph_dragging;
    int m_graph_index;

    //! \brief List of all queue text to draw.. not sure why I didn't use a vector here.. Might of been a leak issue
    QVector<TextQue> m_textque;

    int m_lastxpos, m_lastypos;

    QString m_emptytext;
    bool m_showsplitter;

    qint64 m_minx, m_maxx;

    QList<SelectionHistoryItem> fwd_history;
    float print_scaleX, print_scaleY;

    QPixmap previous_day_snapshot;
    QPixmap current_day_snapshot;
    bool m_fadingOut;
    bool m_fadingIn;
    bool m_limbo;
    bool m_fadedir;
    bool m_inAnimation;
    bool m_blockUpdates;

    QPoint m_mouse;
    qint64 m_currenttime;


    QTime m_animationStarted;

    bool use_pixmap_cache;

    QPixmapCache pixmapcache;

    QTime horizScrollTime, vertScrollTime;

  public slots:
    //! \brief Callback from the ScrollBar, to change scroll position
    void scrollbarValueChanged(int val);

    //! \brief Simply refreshes the GL view, called when timeout expires.
    void refreshTimeout();

    //! \brief Call UpdateGL unless animation is in progress
    void redraw();

};

#endif // GGRAPHVIEW_H
