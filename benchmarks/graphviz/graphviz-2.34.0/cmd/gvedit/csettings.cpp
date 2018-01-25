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
#ifdef WIN32
#include "windows.h"
#endif
#include "csettings.h"
#include "qmessagebox.h"
#include "qfiledialog.h"
#include <QtGui>
#include <qfile.h>
#include "mdichild.h"
#include "string.h"
#include "mainwindow.h"
#include <QTemporaryFile>

extern int errorPipe(char *errMsg);

#define WIDGET(t,f)  ((t*)findChild<t *>(#f))
typedef struct {
    const char *data;
    int len;
    int cur;
} rdr_t;

bool loadAttrs(const QString fileName, QComboBox * cbNameG,
	       QComboBox * cbNameN, QComboBox * cbNameE)
{
    QStringList lines;
    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly)) {
	QTextStream stream(&file);
	QString line;
	while (!stream.atEnd()) {
	    line = stream.readLine();	// line of text excluding '\n'
	    if (line.left(1) == ":") {
		QString attrName;
		QStringList sl = line.split(":");
		for (int id = 0; id < sl.count(); id++) {
		    if (id == 1)
			attrName = sl[id];
		    if (id == 2) {
			if (sl[id].contains("G"))
			    cbNameG->addItem(attrName);
			if (sl[id].contains("N"))
			    cbNameN->addItem(attrName);
			if (sl[id].contains("E"))
			    cbNameE->addItem(attrName);
		    }
		};
	    }
	}
	file.close();
    } else {
	errout << "Could not open attribute name file \"" << fileName <<
	    "\" for reading\n" << flush;
    }

    return false;
}

QString stripFileExtension(QString fileName)
{
    int idx;
    for (idx = fileName.length(); idx >= 0; idx--) {
	if (fileName.mid(idx, 1) == ".")
	    break;
    }
    return fileName.left(idx);
}

#ifndef WITH_CGRAPH
static char*
graph_reader(char *str, int num, FILE * stream)	//helper function to load / parse graphs from tstring
{
    if (num == 0)
	return str;
    const char *ptr;
    char *optr;
    char c;
    int l;
    rdr_t *s = (rdr_t *) stream;
    if (s->cur >= s->len)
	return NULL;
    l = 0;
    ptr = s->data + s->cur;
    optr = str;
    do {
	*optr++ = c = *ptr++;
	l++;
    } while (c && (c != '\n') && (l < num - 1));
    *optr = '\0';
    s->cur += l;
    return str;
}
#endif

CFrmSettings::CFrmSettings()
{
    this->gvc = gvContext();
    Ui_Dialog tempDia;
    tempDia.setupUi(this);
    graph = NULL;
    activeWindow = NULL;
    QString path;
#ifndef WIN32
    char *s = getenv("GVEDIT_PATH");
    if (s)
	path = s;
    else
	path = GVEDIT_DATADIR;
#endif

    connect(WIDGET(QPushButton, pbAdd), SIGNAL(clicked()), this,
	    SLOT(addSlot()));
    connect(WIDGET(QPushButton, pbNew), SIGNAL(clicked()), this,
	    SLOT(newSlot()));
    connect(WIDGET(QPushButton, pbOpen), SIGNAL(clicked()), this,
	    SLOT(openSlot()));
    connect(WIDGET(QPushButton, pbSave), SIGNAL(clicked()), this,
	    SLOT(saveSlot()));
    connect(WIDGET(QPushButton, btnOK), SIGNAL(clicked()), this,
	    SLOT(okSlot()));
    connect(WIDGET(QPushButton, btnCancel), SIGNAL(clicked()), this,
	    SLOT(cancelSlot()));
    connect(WIDGET(QPushButton, pbOut), SIGNAL(clicked()), this,
	    SLOT(outputSlot()));
    connect(WIDGET(QPushButton, pbHelp), SIGNAL(clicked()), this,
	    SLOT(helpSlot()));

    connect(WIDGET(QComboBox, cbScope), SIGNAL(currentIndexChanged(int)),
	    this, SLOT(scopeChangedSlot(int)));
    scopeChangedSlot(0);


#ifndef WIN32
    loadAttrs(path + "/attrs.txt", WIDGET(QComboBox, cbNameG),
	      WIDGET(QComboBox, cbNameN), WIDGET(QComboBox, cbNameE));
#else
    loadAttrs("../share/graphviz/gvedit/attributes.txt",
	      WIDGET(QComboBox, cbNameG), WIDGET(QComboBox, cbNameN),
	      WIDGET(QComboBox, cbNameE));
#endif
    setWindowIcon(QIcon(":/images/icon.png"));
}

void CFrmSettings::outputSlot()
{
    QString _filter =
	"Output File(*." + WIDGET(QComboBox,
				  cbExtension)->currentText() + ")";
    QString fileName =
	QFileDialog::getSaveFileName(this, tr("Save Graph As.."), "/",
				     _filter);
    if (!fileName.isEmpty())
	WIDGET(QLineEdit, leOutput)->setText(fileName);
}

void CFrmSettings::scopeChangedSlot(int id)
{
    WIDGET(QComboBox, cbNameG)->setVisible(id == 0);
    WIDGET(QComboBox, cbNameN)->setVisible(id == 1);
    WIDGET(QComboBox, cbNameE)->setVisible(id == 2);
}

void CFrmSettings::addSlot()
{
    QString _scope = WIDGET(QComboBox, cbScope)->currentText();
    QString _name;
    switch (WIDGET(QComboBox, cbScope)->currentIndex()) {
    case 0:
	_name = WIDGET(QComboBox, cbNameG)->currentText();
	break;
    case 1:
	_name = WIDGET(QComboBox, cbNameN)->currentText();
	break;
    case 2:
	_name = WIDGET(QComboBox, cbNameE)->currentText();
	break;
    }
    QString _value = WIDGET(QLineEdit, leValue)->text();

    if (_value.trimmed().length() == 0)
	QMessageBox::warning(this, tr("GvEdit"),
			     tr
			     ("Please enter a value for selected attribute!"),
			     QMessageBox::Ok, QMessageBox::Ok);
    else {
	QString str = _scope + "[" + _name + "=\"";
	if (WIDGET(QTextEdit, teAttributes)->toPlainText().contains(str)) {
	    QMessageBox::warning(this, tr("GvEdit"),
				 tr("Attribute is already defined!"),
				 QMessageBox::Ok, QMessageBox::Ok);
	    return;
	} else {
	    str = str + _value + "\"]";
	    WIDGET(QTextEdit,
		   teAttributes)->setPlainText(WIDGET(QTextEdit,
						      teAttributes)->
					       toPlainText() + str + "\n");

	}
    }
}

void CFrmSettings::helpSlot()
{
    QDesktopServices::
	openUrl(QUrl("http://www.graphviz.org/doc/info/attrs.html"));
}

void CFrmSettings::cancelSlot()
{
    this->reject();
}

void CFrmSettings::okSlot()
{
    saveContent();
    this->done(drawGraph());
}

void CFrmSettings::newSlot()
{
    WIDGET(QTextEdit, teAttributes)->setPlainText(tr(""));
}

void CFrmSettings::openSlot()
{
    QString fileName =
	QFileDialog::getOpenFileName(this, tr("Open File"), "/",
				     tr("Text file (*.*)"));
    if (!fileName.isEmpty()) {
	QFile file(fileName);
	if (!file.open(QFile::ReadOnly | QFile::Text)) {
	    QMessageBox::warning(this, tr("MDI"),
				 tr("Cannot read file %1:\n%2.")
				 .arg(fileName)
				 .arg(file.errorString()));
	    return;
	}

	QTextStream in(&file);
	WIDGET(QTextEdit, teAttributes)->setPlainText(in.readAll());
    }

}

void CFrmSettings::saveSlot()
{

    if (WIDGET(QTextEdit, teAttributes)->toPlainText().trimmed().
	length() == 0) {
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
	out << WIDGET(QTextEdit, teAttributes)->toPlainText();
	return;
    }

}

bool CFrmSettings::loadGraph(MdiChild * m)
{
    if (graph) {
	agclose(graph);
	graph = NULL;
    }
    graphData.clear();
    graphData.append(m->toPlainText());
    setActiveWindow(m);
    return true;

}

bool CFrmSettings::createLayout()
{
    rdr_t rdr;
    //first attach attributes to graph
    int _pos = graphData.indexOf(tr("{"));
    graphData.replace(_pos, 1,
		      "{" + WIDGET(QTextEdit,
				   teAttributes)->toPlainText());

    /* Reset line number and file name;
     * If known, might want to use real name
     */
    agsetfile((char*)"<gvedit>");
    QByteArray bytes = graphData.toUtf8();
    rdr.data = bytes.constData();
    rdr.len = strlen(rdr.data);
    rdr.cur = 0;
#ifdef WITH_CGRAPH
    graph = agmemread(rdr.data);
#else
    graph = agread_usergets((FILE *) & rdr, (gets_f) graph_reader);
    /* graph=agread_usergets(reinterpret_cast<FILE*>(this),(gets_f)graph_reader); */
#endif
    if (!graph)
	return false;
    if (agerrors()) {
	agclose(graph);
	graph = NULL;
	return false;
    }
    Agraph_t *G = this->graph;
    QString layout;

    if(agfindnodeattr(G, (char*)"pos"))
	layout="nop2";
    else
	layout=WIDGET(QComboBox, cbLayout)->currentText();


    gvLayout(gvc, G, (char *)layout.toUtf8().constData());	/* library function */
    return true;
}

static QString buildTempFile()
{
    QTemporaryFile tempFile;
    tempFile.setAutoRemove(false);
    tempFile.open();
    QString a = tempFile.fileName();
    tempFile.close();
    return a;
}

void CFrmSettings::doPreview(QString fileName)
{
    if (getActiveWindow()->previewFrm) {
	getActiveWindow()->parentFrm->mdiArea->
	    removeSubWindow(getActiveWindow()->previewFrm->subWindowRef);
	delete getActiveWindow()->previewFrm;
	getActiveWindow()->previewFrm = NULL;
    }

    if ((fileName.isNull()) || !(getActiveWindow()->loadPreview(fileName))) {	//create preview
	QString prevFile(buildTempFile());
	gvRenderFilename(gvc, graph, "png",
			 (char *) prevFile.toUtf8().constData());
	getActiveWindow()->loadPreview(prevFile);
#if 0
	if (!this->getActiveWindow()->loadPreview(prevFile))
	    QMessageBox::information(this, tr("GVEdit"),
				     tr
				     ("Preview file can not be opened."));
#endif
    }
}

bool CFrmSettings::renderLayout()
{
    if (!graph)
	return false;
    QString sfx = WIDGET(QComboBox, cbExtension)->currentText();
    QString fileName(WIDGET(QLineEdit, leOutput)->text());

    if ((fileName == QString("")) || (sfx == QString("NONE")))
	doPreview(QString());
    else {
	fileName = stripFileExtension(fileName);
	fileName = fileName + "." + sfx;
	if (fileName != activeWindow->outputFile)
	    activeWindow->outputFile = fileName;

#ifdef WIN32
	if ((!fileName.contains('/')) && (!fileName.contains('\\'))) 
#else
	if (!fileName.contains('/'))
#endif
	{  // no directory info => can we create/write the file?
	    QFile outf(fileName);
	    if (outf.open(QIODevice::WriteOnly))
		outf.close();
	    else {
		QString pathName = QDir::homePath();
		pathName.append("/").append(fileName);
		fileName = QDir::toNativeSeparators (pathName);
		QString msg ("Output written to ");
		msg.append(fileName);
		msg.append("\n");
		errorPipe((char *) msg.toAscii().constData());
	    }
	}

	if (gvRenderFilename
	    (gvc, graph, (char *) sfx.toUtf8().constData(),
	     (char *) fileName.toUtf8().constData()))
	    return false;

	doPreview(fileName);
    }
    return true;
}



bool CFrmSettings::loadLayouts()
{
    return false;
}

bool CFrmSettings::loadRenderers()
{
    return false;
}

void CFrmSettings::refreshContent()
{

    WIDGET(QComboBox, cbLayout)->setCurrentIndex(activeWindow->layoutIdx);
    WIDGET(QComboBox,
	   cbExtension)->setCurrentIndex(activeWindow->renderIdx);
    if (!activeWindow->outputFile.isEmpty())
	WIDGET(QLineEdit, leOutput)->setText(activeWindow->outputFile);
    else
	WIDGET(QLineEdit,
	       leOutput)->setText(stripFileExtension(activeWindow->
						     currentFile()) + "." +
				  WIDGET(QComboBox,
					 cbExtension)->currentText());

    WIDGET(QTextEdit, teAttributes)->setText(activeWindow->attributes);

    WIDGET(QLineEdit, leValue)->setText("");

}

void CFrmSettings::saveContent()
{
    activeWindow->layoutIdx = WIDGET(QComboBox, cbLayout)->currentIndex();
    activeWindow->renderIdx =
	WIDGET(QComboBox, cbExtension)->currentIndex();
    activeWindow->outputFile = WIDGET(QLineEdit, leOutput)->text();
    activeWindow->attributes =
	WIDGET(QTextEdit, teAttributes)->toPlainText();
}

int CFrmSettings::drawGraph()
{
    int rc;
    if (createLayout() && renderLayout()) {
	getActiveWindow()->settingsSet = false;
	rc = QDialog::Accepted;
    } else
	rc = QDialog::Accepted;
    agreseterrors();

    return rc;
    /* return QDialog::Rejected; */
}

int CFrmSettings::runSettings(MdiChild * m)
{
	if (this->loadGraph(m))
	    return drawGraph();


    if ((m) && (m == getActiveWindow())) {
	if (this->loadGraph(m))
	    return drawGraph();
	else
	    return QDialog::Rejected;
    }

    else
	return showSettings(m);

}

int CFrmSettings::showSettings(MdiChild * m)
{

    if (this->loadGraph(m)) {
	refreshContent();
	return this->exec();
    } else
	return QDialog::Rejected;
}

void CFrmSettings::setActiveWindow(MdiChild * m)
{
    this->activeWindow = m;

}

MdiChild *CFrmSettings::getActiveWindow()
{
    return activeWindow;
}
