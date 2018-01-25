/* $Id$Revision: */
/* vim:set shiftwidth=4 ts=8: */

/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: See CVS logs. Details at http://www.graphviz.org/
 *************************************************************************/

#define QT_NO_PRINTER 1

#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H
#include <QtGui>
#include <QMainWindow>
#include <QPrinter>

QT_BEGIN_NAMESPACE
class QAction;
class QLabel;
class QMenu;
class QScrollArea;
class QScrollBar;
class MdiChild;
QT_END_NAMESPACE

//! [0]
class ImageViewer : public QMainWindow
{
    Q_OBJECT

public:
    ImageViewer();
    MdiChild* graphWindow;
    QMdiSubWindow* subWindowRef; //reference to its wrapping sub window
public slots:
    bool open(QString fileName);
    void print();
    void zoomIn();
    void zoomOut();
    void normalSize();
    void fitToWindow();
    void about();


private slots:

private:
    void createActions();
    void createMenus();
    void updateActions();
    void scaleImage(double factor);
    void adjustScrollBar(QScrollBar *scrollBar, double factor);

    QLabel *imageLabel;
    QScrollArea *scrollArea;
    double scaleFactor;

#ifndef QT_NO_PRINTER
    QPrinter printer;
#endif

    QAction *openAct;
    QAction *printAct;
    QAction *exitAct;
    QAction *zoomInAct;
    QAction *zoomOutAct;
    QAction *normalSizeAct;
    QAction *fitToWindowAct;
    QAction *aboutAct;
    QAction *aboutQtAct;

//    QMenu *fileMenu;
    QMenu *viewMenu;
//    QMenu *helpMenu;
	protected:
        void closeEvent(QCloseEvent *event);

};
//! [0]

#endif
