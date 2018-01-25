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

#include <QtGui>
#include <qframe.h>
#include "mainwindow.h"
#include "mdichild.h"
#include "csettings.h"
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include	<string.h>

QTextEdit *globTextEdit;

int errorPipe(char *errMsg)
{
    globTextEdit->setText(globTextEdit->toPlainText() + QString(errMsg));
    return 0;
}

static void freeList(char **lp, int count)
{
    for (int i = 0; i < count; i++)
	free(lp[i]);
    free(lp);
}

static int LoadPlugins(QComboBox * cb, GVC_t * gvc, char *kind,
		       char *more[], char *prefer)
{
    int count;
    char **lp = gvPluginList(gvc, kind, &count, NULL);
    int idx = -1;

    cb->clear();
    for (int id = 0; id < count; id++) {
	cb->addItem(QString(lp[id]));
	if (prefer && (idx < 0) && !strcmp(prefer, lp[id]))
	    idx = id;
    };
    freeList(lp, count);

    /* Add additional items if supplied */
    if (more) {
	int i = 0;
	char *s;
	while ((s = more[i++])) {
	    cb->addItem(QString(s));
	}
    }

    if (idx > 0)
	cb->setCurrentIndex(idx);
    else
	idx = 0;

    return idx;
}

void CMainWindow::createConsole()
{
    QDockWidget *dock = new QDockWidget(tr("Output Console"), NULL);
    QTextEdit *textEdit = new QTextEdit(dock);

    dock->
	setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
//    dock->setWidget(textEdit);
    addDockWidget(Qt::BottomDockWidgetArea, dock);
    QVBoxLayout *vL = new QVBoxLayout();


    textEdit->setObjectName(QString::fromUtf8("textEdit"));
/*    textEdit->setMinimumSize(QSize(0, 80));
    textEdit->setMaximumSize(QSize(16777215, 120));*/
    globTextEdit = textEdit;
    agseterrf(errorPipe);

    vL->addWidget(textEdit);
    vL->setContentsMargins(1, 1, 1, 1);

    QFrame *fr = new QFrame(dock);
    vL->addWidget(fr);

    QPushButton *logNewBtn =
	new QPushButton(QIcon(":/images/new.png"), "", fr);
    QPushButton *logSaveBtn =
	new QPushButton(QIcon(":/images/save.png"), "", fr);
    QHBoxLayout *consoleLayout = new QHBoxLayout();
    consoleLayout->addWidget(logNewBtn);
    connect(logNewBtn, SIGNAL(clicked()), this, SLOT(slotNewLog()));
    connect(logSaveBtn, SIGNAL(clicked()), this, SLOT(slotSaveLog()));
    consoleLayout->addWidget(logSaveBtn);
    consoleLayout->addStretch();

    consoleLayout->setContentsMargins(1, 1, 1, 1);;
    consoleLayout->setContentsMargins(1, 1, 1, 1);

    fr->setLayout(consoleLayout);

    QFrame *mainFrame = new QFrame(dock);
    mainFrame->setLayout(vL);


    dock->setWidget(mainFrame);

}

static char *xtra[] = {
    "NONE",
    (char *) 0
};

CMainWindow::CMainWindow(char*** Files)
{

    QWidget *centralwidget = new QWidget(this);
    centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
    QVBoxLayout *verticalLayout_2 = new QVBoxLayout(centralwidget);
    verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
    QVBoxLayout *verticalLayout = new QVBoxLayout();
    verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
    mdiArea = new QMdiArea(centralwidget);
    mdiArea->setObjectName(QString::fromUtf8("mdiArea"));

    verticalLayout->addWidget(mdiArea);
    verticalLayout_2->setContentsMargins(1, 1, 1, 1);
    verticalLayout_2->addLayout(verticalLayout);
    setCentralWidget(centralwidget);
    centralwidget->layout()->setContentsMargins(1, 1, 1, 1);
    prevChild = NULL;

    createConsole();






    connect(mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow *)),
	    this, SLOT(slotRefreshMenus()));
    windowMapper = new QSignalMapper(this);
    connect(windowMapper, SIGNAL(mapped(QWidget *)),
	    this, SLOT(activateChild(QWidget *)));

    frmSettings = new CFrmSettings();

    actions();
    menus();
    toolBars();
    statusBar();
    updateMenus();

    readSettings();

    setWindowTitle(tr("GVEdit For Graphviz ver:1.02"));
    this->resize(1024, 900);
    this->move(0, 0);
    setUnifiedTitleAndToolBarOnMac(true);
//    (QComboBox*)frmSettings->findChild<QComboBox*>("cbLayout")
    QComboBox *cb =
	(QComboBox *) frmSettings->findChild < QComboBox * >("cbLayout");
    dfltLayoutIdx = LoadPlugins(cb, frmSettings->gvc, "layout", 0, "dot");
    cb = (QComboBox *) frmSettings->findChild <
	QComboBox * >("cbExtension");
    dfltRenderIdx =
	LoadPlugins(cb, frmSettings->gvc, "device", xtra, "png");
    statusBar()->showMessage(tr("Ready"));
    setWindowIcon(QIcon(":/images/icon.png"));
    //load files specified in command line , one time task
    char** files=*Files;
    if (files)
	while (*files) {
	    addFile(QString(*files));
	    files++;
	}



}

void CMainWindow::closeEvent(QCloseEvent * event)
{
    mdiArea->closeAllSubWindows();
    if (mdiArea->currentSubWindow()) {
	event->ignore();
    } else {
	writeSettings();
	event->accept();
    }
}

void CMainWindow::slotNew()
{
    MdiChild *child = createMdiChild();
    child->newFile();
    child->show();
}

void CMainWindow::addFile(QString fileName)
{
    if (!fileName.isEmpty()) {
	QMdiSubWindow *existing = findMdiChild(fileName);
	if (existing) {
	    mdiArea->setActiveSubWindow(existing);
	    return;
	}

	MdiChild *child = createMdiChild();
	if (child->loadFile(fileName)) {
	    statusBar()->showMessage(tr("File loaded"), 2000);
	    child->show();
	    slotRun(child);
	} else {
	    child->close();
	}
    }
}

void CMainWindow::slotOpen()
{
    QStringList filters;
    filters << "*.cpp" << "*.cxx" << "*.cc";

    QFileDialog fd;
//    fd.setProxyModel(new FileFilterProxyModel());
    fd.setNameFilter("XML (*.xml)");
    QString fileName = fd.getOpenFileName(this);
//    QFileDialog::getOpenFileName(this);

    addFile(fileName);
}

void CMainWindow::slotSave()
{
    if (activeMdiChild() && activeMdiChild()->save())
	statusBar()->showMessage(tr("File saved"), 2000);
}

void CMainWindow::slotSaveAs()
{
    if (activeMdiChild() && activeMdiChild()->saveAs())
	statusBar()->showMessage(tr("File saved"), 2000);
}

void CMainWindow::slotCut()
{
    if (activeMdiChild())
	activeMdiChild()->cut();
}

void CMainWindow::slotCopy()
{
    if (activeMdiChild())
	activeMdiChild()->copy();
}

void CMainWindow::slotPaste()
{
    if (activeMdiChild())
	activeMdiChild()->paste();
}

void CMainWindow::slotAbout()
{
    QMessageBox::about(this, tr("About GVEdit\n"),
		       tr("<b>GVEdit</b> Graph File Editor For Graphviz\n"
			  "Version:1.02"));
}

void CMainWindow::setChild ()
{
    if (prevChild != activeMdiChild()) {
	QString msg;
	msg.append("working on ");
	msg.append(activeMdiChild()->currentFile());
	msg.append("\n");
	errorPipe((char *) msg.toAscii().constData());
	prevChild = activeMdiChild();
    }
}

void CMainWindow::slotSettings()
{
    setChild ();
    frmSettings->showSettings(activeMdiChild());
}

void CMainWindow::slotRun(MdiChild * m)
{
    setChild ();

    
    
//    if ((activeMdiChild()) && (!activeMdiChild()->firstTime()))
    if(m)
	frmSettings->runSettings(m);
    else
	frmSettings->runSettings(activeMdiChild());
//    if ((activeMdiChild()) && (activeMdiChild()->firstTime()))
//	frmSettings->showSettings(activeMdiChild());


}



void CMainWindow::slotNewLog()
{
    globTextEdit->clear();
}

void CMainWindow::slotSaveLog()
{

    if (globTextEdit->toPlainText().trimmed().length() == 0) {
	QMessageBox::warning(this, tr("GvEdit"), tr("Nothing to save!"),
			     QMessageBox::Ok, QMessageBox::Ok);
	return;


    }

    QString fileName = QFileDialog::getSaveFileName(this, tr("Open File"),
						    "/",
						    tr("Text File(*.*)"));
    if (!fileName.isEmpty()) {

	QFile file(fileName);
	if (!file.open(QFile::WriteOnly | QFile::Text)) {
	    QMessageBox::warning(this, tr("MDI"),
				 tr("Cannot write file %1:\n%2.")
				 .arg(fileName)
				 .arg(file.errorString()));
	    return;
	}

	QTextStream out(&file);
	out << globTextEdit->toPlainText();
	return;
    }


}

void CMainWindow::updateFileMenu()
{
    if (!activeMdiChild()) {
	saveAct->setEnabled(false);
	saveAsAct->setEnabled(false);
	pasteAct->setEnabled(false);
	closeAct->setEnabled(false);
	closeAllAct->setEnabled(false);
	tileAct->setEnabled(false);
	cascadeAct->setEnabled(false);
	nextAct->setEnabled(false);
	previousAct->setEnabled(false);
	separatorAct->setVisible(false);
	settingsAct->setEnabled(false);
	layoutAct->setEnabled(false);
    } else {
	saveAct->setEnabled(true);
	saveAsAct->setEnabled(true);
	pasteAct->setEnabled(true);
	closeAct->setEnabled(true);
	closeAllAct->setEnabled(true);
	tileAct->setEnabled(true);
	cascadeAct->setEnabled(true);
	nextAct->setEnabled(true);
	previousAct->setEnabled(true);
	separatorAct->setVisible(true);
	settingsAct->setEnabled(true);
	layoutAct->setEnabled(true);

	if (activeMdiChild()->textCursor().hasSelection()) {
	    cutAct->setEnabled(true);
	    copyAct->setEnabled(true);

	} else {
	    cutAct->setEnabled(false);
	    copyAct->setEnabled(false);

	}



    }

}

void CMainWindow::slotRefreshMenus()
{
    updateMenus();
}

void CMainWindow::updateMenus()
{
    this->updateFileMenu();
    this->updateWindowMenu();

}

void CMainWindow::updateWindowMenu()
{
    mWindow->clear();
    mWindow->addAction(closeAct);
    mWindow->addAction(closeAllAct);
    mWindow->addSeparator();
    mWindow->addAction(tileAct);
    mWindow->addAction(cascadeAct);
    mWindow->addSeparator();
    mWindow->addAction(nextAct);
    mWindow->addAction(previousAct);
    mWindow->addAction(separatorAct);

    QList < QMdiSubWindow * >windows = mdiArea->subWindowList();
    separatorAct->setVisible(!windows.isEmpty());

    for (int i = 0; i < windows.size(); ++i) {
	if (windows.at(i)->widget()->inherits("MdiChild")) {
	    MdiChild *child =
		qobject_cast < MdiChild * >(windows.at(i)->widget());
	    QString text;
	    if (i < 9) {
		text = tr("&%1 %2").arg(i + 1)
		    .arg(child->userFriendlyCurrentFile());
	    } else {
		text = tr("%1 %2").arg(i + 1)
		    .arg(child->userFriendlyCurrentFile());
	    }
	    QAction *action = mWindow->addAction(text);
	    action->setCheckable(true);
	    action->setChecked(child == activeMdiChild());
	    connect(action, SIGNAL(triggered()), windowMapper,
		    SLOT(map()));
	    windowMapper->setMapping(action, windows.at(i));
	}
    }
}

MdiChild *CMainWindow::createMdiChild()
{
    MdiChild *child = new MdiChild;
    child->parentFrm = this;
    QMdiSubWindow *s = mdiArea->addSubWindow(child);
    s->resize(800, 600);
    s->move(mdiArea->subWindowList().count() * 5,
	    mdiArea->subWindowList().count() * 5);
    connect(child, SIGNAL(copyAvailable(bool)), cutAct,
	    SLOT(setEnabled(bool)));
    connect(child, SIGNAL(copyAvailable(bool)), copyAct,
	    SLOT(setEnabled(bool)));
    child->layoutIdx = dfltLayoutIdx;
    child->renderIdx = dfltRenderIdx;

    return child;
}

void CMainWindow::actions()
{
    newAct = new QAction(QIcon(":/images/new.png"), tr("&New"), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("Create a new file"));
    connect(newAct, SIGNAL(triggered()), this, SLOT(slotNew()));

    openAct =
	new QAction(QIcon(":/images/open.png"), tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, SIGNAL(triggered()), this, SLOT(slotOpen()));

    saveAct = new QAction(QIcon(":/images/save.png"), tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save the document to disk"));
    connect(saveAct, SIGNAL(triggered()), this, SLOT(slotSave()));

    saveAsAct = new QAction(tr("Save &As..."), this);
    saveAsAct->setShortcuts(QKeySequence::SaveAs);
    saveAsAct->setStatusTip(tr("Save the document under a new name"));
    connect(saveAsAct, SIGNAL(triggered()), this, SLOT(slotSaveAs()));
    exitAct = new QAction(tr("E&xit"), this);
#if (QT_VERSION >= QT_VERSION_CHECK(4, 6, 0))
    exitAct->setShortcuts(QKeySequence::Quit);
#endif
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));
    cutAct = new QAction(QIcon(":/images/cut.png"), tr("Cu&t"), this);
    cutAct->setShortcuts(QKeySequence::Cut);
    cutAct->
	setStatusTip(tr
		     ("Cut the current selection's contents to the "
		      "clipboard"));
    connect(cutAct, SIGNAL(triggered()), this, SLOT(slotCut()));

    copyAct = new QAction(QIcon(":/images/copy.png"), tr("&Copy"), this);
    copyAct->setShortcuts(QKeySequence::Copy);
    copyAct->
	setStatusTip(tr
		     ("Copy the current selection's contents to the "
		      "clipboard"));
    connect(copyAct, SIGNAL(triggered()), this, SLOT(slotCopy()));

    pasteAct =
	new QAction(QIcon(":/images/paste.png"), tr("&Paste"), this);
    pasteAct->setShortcuts(QKeySequence::Paste);
    pasteAct->
	setStatusTip(tr
		     ("Paste the clipboard's contents into the current "
		      "selection"));
    connect(pasteAct, SIGNAL(triggered()), this, SLOT(slotPaste()));

    closeAct = new QAction(tr("Cl&ose"), this);
    closeAct->setStatusTip(tr("Close the active window"));
    connect(closeAct, SIGNAL(triggered()), mdiArea,
	    SLOT(closeActiveSubWindow()));

    closeAllAct = new QAction(tr("Close &All"), this);
    closeAllAct->setStatusTip(tr("Close all the windows"));
    connect(closeAllAct, SIGNAL(triggered()), mdiArea,
	    SLOT(closeAllSubWindows()));

    tileAct = new QAction(tr("&Tile"), this);
    tileAct->setStatusTip(tr("Tile the windows"));
    connect(tileAct, SIGNAL(triggered()), mdiArea, SLOT(tileSubWindows()));

    cascadeAct = new QAction(tr("&Cascade"), this);
    cascadeAct->setStatusTip(tr("Cascade the windows"));
    connect(cascadeAct, SIGNAL(triggered()), mdiArea,
	    SLOT(cascadeSubWindows()));

    nextAct = new QAction(tr("Ne&xt"), this);
    nextAct->setShortcuts(QKeySequence::NextChild);
    nextAct->setStatusTip(tr("Move the focus to the next window"));
    connect(nextAct, SIGNAL(triggered()), mdiArea,
	    SLOT(activateNextSubWindow()));

    previousAct = new QAction(tr("Pre&vious"), this);
    previousAct->setShortcuts(QKeySequence::PreviousChild);
    previousAct->setStatusTip(tr("Move the focus to the previous window"));
    connect(previousAct, SIGNAL(triggered()), mdiArea,
	    SLOT(activatePreviousSubWindow()));

    separatorAct = new QAction(this);
    separatorAct->setSeparator(true);

    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(slotAbout()));



    settingsAct =
	new QAction(QIcon(":/images/settings.png"), tr("Settings"), this);
    settingsAct->setStatusTip(tr("Show Graphviz Settings"));
    connect(settingsAct, SIGNAL(triggered()), this, SLOT(slotSettings()));
    settingsAct->setShortcut(QKeySequence(Qt::SHIFT + Qt::Key_F5));




    layoutAct = new QAction(QIcon(":/images/run.png"), tr("Layout"), this);
    layoutAct->setStatusTip(tr("Layout the active graph"));
    connect(layoutAct, SIGNAL(triggered()), this, SLOT(slotRun()));
    layoutAct->setShortcut(QKeySequence(Qt::Key_F5));



}

void CMainWindow::menus()
{
    mFile = menuBar()->addMenu(tr("&File"));
    mEdit = menuBar()->addMenu(tr("&Edit"));
    mWindow = menuBar()->addMenu(tr("&Window"));
    mGraph = menuBar()->addMenu(tr("&Graph"));
    mHelp = menuBar()->addMenu(tr("&Help"));


    mFile->addAction(newAct);
    mFile->addAction(openAct);
    mFile->addAction(saveAct);
    mFile->addAction(saveAsAct);
    mFile->addSeparator();

    mFile->addAction(exitAct);

    mEdit->addAction(cutAct);
    mEdit->addAction(copyAct);
    mEdit->addAction(pasteAct);

    mGraph->addAction(settingsAct);
    mGraph->addAction(layoutAct);
    mGraph->addSeparator();
    loadPlugins();

    updateWindowMenu();
    connect(mWindow, SIGNAL(aboutToShow()), this,
	    SLOT(slotRefreshMenus()));
    mHelp->addAction(aboutAct);
}

void CMainWindow::toolBars()
{
    tbFile = addToolBar(tr("File"));
    tbFile->addAction(newAct);
    tbFile->addAction(openAct);
    tbFile->addAction(saveAct);

    tbEdit = addToolBar(tr("Edit"));
    tbEdit->addAction(cutAct);
    tbEdit->addAction(copyAct);
    tbEdit->addAction(pasteAct);

    tbGraph = addToolBar(tr("Graph"));
    tbGraph->addAction(settingsAct);
    tbGraph->addAction(layoutAct);

}


void CMainWindow::readSettings()
{
    QSettings settings("Trolltech", "MDI Example");
    QPoint pos = settings.value("pos", QPoint(200, 200)).toPoint();
    QSize size = settings.value("size", QSize(400, 400)).toSize();
    move(pos);
    resize(size);
}

void CMainWindow::writeSettings()
{
    QSettings settings("Trolltech", "MDI Example");
    settings.setValue("pos", pos());
    settings.setValue("size", size());
}

MdiChild *CMainWindow::activeMdiChild()
{
    if (QMdiSubWindow * activeSubWindow = mdiArea->activeSubWindow()) {
	if (activeSubWindow->widget()->inherits("MdiChild"))
	    return qobject_cast < MdiChild * >(activeSubWindow->widget());
	else
	    return qobject_cast <
		ImageViewer * >(activeSubWindow->widget())->graphWindow;

    }
    return 0;
}

QMdiSubWindow *CMainWindow::findMdiChild(const QString & fileName)
{
    QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

    foreach(QMdiSubWindow * window, mdiArea->subWindowList()) {
	if (window->widget()->inherits("MdiChild")) {

	    MdiChild *mdiChild =
		qobject_cast < MdiChild * >(window->widget());
	    if (mdiChild->currentFile() == canonicalFilePath)
		return window;
	} else {

	    MdiChild *mdiChild =
		qobject_cast <
		ImageViewer * >(window->widget())->graphWindow;
	    if (mdiChild->currentFile() == canonicalFilePath)
		return window;
	}



    }
    return 0;
}

void CMainWindow::activateChild(QWidget * window)
{
    if (!window)
	return;
    mdiArea->setActiveSubWindow(qobject_cast < QMdiSubWindow * >(window));
}

void CMainWindow::loadPlugins()
{

}
