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

#include "mdichild.h"
#include "mainwindow.h"

MdiChild::MdiChild()
{
    setAttribute(Qt::WA_DeleteOnClose);
    isUntitled = true;
    layoutIdx = 0;
    renderIdx = 0;
    preview = true;
    applyCairo = false;
    previewFrm = NULL;
    settingsSet = false;
}

void MdiChild::newFile()
{
    static int sequenceNumber = 1;

    isUntitled = true;
    curFile = tr("graph%1.gv").arg(sequenceNumber++);
    setWindowTitle(curFile + "[*]");

    connect(document(), SIGNAL(contentsChanged()),
	    this, SLOT(documentWasModified()));
}

bool MdiChild::loadFile(const QString & fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
	QMessageBox::warning(this, tr("MDI"),
			     tr("Cannot read file %1:\n%2.")
			     .arg(fileName)
			     .arg(file.errorString()));
	return false;
    }

    QTextStream in(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    setPlainText(in.readAll());
    QApplication::restoreOverrideCursor();

    setCurrentFile(fileName);

    connect(document(), SIGNAL(contentsChanged()),
	    this, SLOT(documentWasModified()));

    return true;
}

bool MdiChild::save()
{
    if (isUntitled) {
	return saveAs();
    } else {
	return saveFile(curFile);
    }
}

bool MdiChild::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"),
						    curFile);
    if (fileName.isEmpty())
	return false;

    return saveFile(fileName);
}

bool MdiChild::saveFile(const QString & fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
	QMessageBox::warning(this, tr("MDI"),
			     tr("Cannot write file %1:\n%2.")
			     .arg(fileName)
			     .arg(file.errorString()));
	return false;
    }

    QTextStream out(&file);
    QApplication::setOverrideCursor(Qt::WaitCursor);
//    out << toPlainText();
//    out << toPlainText().toUtf8().constData();
    out.setCodec("UTF-8");
    out << toPlainText();
    out.flush();
    QApplication::restoreOverrideCursor();

    setCurrentFile(fileName);
    return true;
}

QString MdiChild::userFriendlyCurrentFile()
{
    return strippedName(curFile);
}

void MdiChild::closeEvent(QCloseEvent * event)
{
    if (maybeSave()) {
	event->accept();
    } else {
	event->ignore();
    }
}

void MdiChild::documentWasModified()
{
    setWindowModified(document()->isModified());
}

bool MdiChild::maybeSave()
{
    if (document()->isModified()) {
	QMessageBox::StandardButton ret;
	ret = QMessageBox::warning(this, tr("MDI"),
				   tr("'%1' has been modified.\n"
				      "Do you want to save your changes?")
				   .arg(userFriendlyCurrentFile()),
				   QMessageBox::Save | QMessageBox::Discard
				   | QMessageBox::Cancel);
	if (ret == QMessageBox::Save)
	    return save();
	else if (ret == QMessageBox::Cancel)
	    return false;
    }
    return true;
}

void MdiChild::setCurrentFile(const QString & fileName)
{
    curFile = QFileInfo(fileName).canonicalFilePath();
    isUntitled = false;
    document()->setModified(false);
    setWindowModified(false);
    setWindowTitle(userFriendlyCurrentFile() + "[*]");
}

QString MdiChild::strippedName(const QString & fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}

bool MdiChild::loadPreview(QString fileName)
{
    if (!this->previewFrm) {
	previewFrm = new ImageViewer();
	previewFrm->graphWindow = this;
	QMdiSubWindow *s = parentFrm->mdiArea->addSubWindow(previewFrm);

	s->resize(600, 400);
	s->move(parentFrm->mdiArea->subWindowList().count() * 5,
		parentFrm->mdiArea->subWindowList().count() * 5);
	previewFrm->subWindowRef = s;

    }
    bool rv = previewFrm->open(fileName);
    if (rv)
	previewFrm->show();
    return rv;

}

bool MdiChild::firstTime()
{
    return settingsSet;
}
