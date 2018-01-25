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


#ifndef CSETTINGS_H
#define CSETTINGS_H

class MdiChild;
#include <QDialog>
#include <QString>
#include "ui_settings.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gvc.h"

class CFrmSettings : public QDialog
{
        Q_OBJECT
public:
    CFrmSettings();
    int runSettings(MdiChild* m);
    int showSettings(MdiChild* m);
    int drawGraph();
    MdiChild* getActiveWindow();
    QString graphData;
    GVC_t* gvc;    
private slots:
    void outputSlot();
    void addSlot();
    void helpSlot();
    void cancelSlot();
    void okSlot();
    void newSlot();
    void openSlot();
    void saveSlot();
    void scopeChangedSlot(int);
private:
    //Actions
    Agraph_t* graph;
    MdiChild* activeWindow;
    QAction* outputAct;
    QAction* addAct;
    QAction* helpAct;
    QAction* cancelAct;
    QAction* okAct;
    QAction* newAct;
    QAction* openAct;
    QAction* saveAct;
    //METHODS
    bool loadLayouts();
    bool loadRenderers();
    void refreshContent();
    void saveContent();
    void doPreview(QString);
    void setActiveWindow(MdiChild* m);
    bool loadGraph(MdiChild* m);
    bool createLayout();
    bool renderLayout();
};

#endif
