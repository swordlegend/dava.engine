/*==================================================================================
    Copyright (c) 2008, binaryzebra
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    * Neither the name of the binaryzebra nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE binaryzebra AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL binaryzebra BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=====================================================================================*/


#include "mainwindow.h"

//////////////////////////////////////////////////////////////////////////
#include "fontmanagerdialog.h"
#include "Helpers/ResourcesManageHelper.h"
#include "Dialogs/LocalizationEditorDialog.h"
//////////////////////////////////////////////////////////////////////////

#include "UI/FileSystemView/FileSystemDockWidget.h"
#include "Utils/QtDavaConvertion.h"

#include "QtTools/FileDialog/FileDialog.h"
#include "QtTools/ConsoleWidget/LogWidget.h"

namespace
{
    const QString APP_GEOMETRY = "geometry";
    const QString APP_STATE = "windowstate";
    const QString CONSOLE_STATE = "console state";
    const char* COLOR_PROPERTY_ID = "color";
}

using namespace DAVA;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , backgroundFrameUseCustomColorAction(nullptr)
    , backgroundFrameSelectCustomColorAction(nullptr)
    , localizationEditorDialog(new LocalizationEditorDialog(this))
{
    setupUi(this);
    actionLocalizationManager->setEnabled(false);
    InitLanguageBox();
    // Console dock
    {
        LogWidget *logWidget = new LogWidget();
        consoleDockWidget->setWidget(logWidget);
        QSettings settings(APP_COMPANY, APP_NAME);
        const auto var = settings.value(CONSOLE_STATE);
        if (var.canConvert<QByteArray>())
        {
            logWidget->Deserialize(var.toByteArray());
        }
    }
    tabBar->setElideMode(Qt::ElideNone);
    setWindowTitle(ResourcesManageHelper::GetProjectTitle());

    tabBar->setTabsClosable(true);
    tabBar->setUsesScrollButtons(true);
    connect(tabBar, &QTabBar::tabCloseRequested, this, &MainWindow::TabClosed);
    connect(tabBar, &QTabBar::currentChanged, this, &MainWindow::OnCurrentIndexChanged);
    connect(tabBar, &QTabBar::currentChanged, this, &MainWindow::CurrentTabChanged);
    setUnifiedTitleAndToolBarOnMac(true);

    connect(actionFontManager, &QAction::triggered, this, &MainWindow::OnOpenFontManager);
    connect(actionLocalizationManager, &QAction::triggered, localizationEditorDialog, &LocalizationEditorDialog::exec);

    connect(fileSystemDockWidget, &FileSystemDockWidget::OpenPackageFile, this, &MainWindow::OpenPackageFile);
    InitMenu();
    RestoreMainWindowState();

    fileSystemDockWidget->setEnabled(false);

    RebuildRecentMenu();
    menuTools->setEnabled(false);
    toolBarPlugins->setEnabled(false);
}

MainWindow::~MainWindow()
{
    SaveMainWindowState();
}

void MainWindow::CreateUndoRedoActions(const QUndoGroup *undoGroup)
{
    Q_ASSERT(undoGroup);
    QAction *undoAction = undoGroup->createUndoAction(this);
    undoAction->setShortcuts(QKeySequence::Undo);
    undoAction->setIcon(QIcon(":/Icons/edit_undo.png"));

    QAction *redoAction = undoGroup->createRedoAction(this);
    redoAction->setShortcuts(QKeySequence::Redo);
    redoAction->setIcon(QIcon(":/Icons/edit_redo.png"));

    mainToolbar->addAction(undoAction);
    mainToolbar->addAction(redoAction);
}

void MainWindow::OnProjectIsOpenChanged(bool arg)
{
    fileSystemDockWidget->setEnabled(arg);
    this->setWindowTitle(ResourcesManageHelper::GetProjectTitle());
}

void MainWindow::OnCountChanged(int count)
{
    actionSaveAllDocuments->setEnabled(count > 0);
    OnCurrentIndexChanged(tabBar->currentIndex());
}

int MainWindow::CloseTab(int index)
{
    delete tabBar->tabData(index).value<TabState*>();
    tabBar->removeTab(index);
    OnCountChanged(tabBar->count());
    return tabBar->currentIndex();
}

void MainWindow::SetCurrentTab(int index)
{
    tabBar->setCurrentIndex(index);
}

void MainWindow::SaveMainWindowState()
{
    QSettings settings(QApplication::organizationName(), QApplication::applicationName());
    settings.setValue(APP_GEOMETRY, saveGeometry());
    settings.setValue(APP_STATE, saveState());
    auto logWidget = qobject_cast<LogWidget*>(consoleDockWidget->widget());
    settings.setValue(CONSOLE_STATE, logWidget->Serialize());
}

void MainWindow::RestoreMainWindowState()
{
    QSettings settings(QApplication::organizationName(), QApplication::applicationName());
    // Check settings befor applying it
    if (settings.value(APP_GEOMETRY).isValid())
    {
        restoreGeometry(settings.value(APP_GEOMETRY).toByteArray());
    }
    if (settings.value(APP_STATE).isValid())
    {
        restoreState(settings.value(APP_STATE).toByteArray());
    }
	QSettings settings(APP_COMPANY, APP_NAME);
    auto val = settings.value(APP_GEOMETRY);
    if (val.canConvert<QByteArray>())
	{
    	restoreGeometry(settings.value(APP_GEOMETRY).toByteArray());
	}
    val = settings.value(APP_STATE);
    if (val.canConvert<QByteArray>())
    {
    	restoreState(settings.value(APP_STATE).toByteArray());
	}
}

DavaGLWidget* MainWindow::GetGLWidget() const
{
    return previewWidget->GetDavaGLWidget();
}

void MainWindow::OnCurrentIndexChanged(int arg)
{
    bool enabled = arg >= 0;
    packageWidget->setEnabled(enabled);
    propertiesWidget->setEnabled(enabled);
    previewWidget->setEnabled(enabled);
    libraryWidget->setEnabled(enabled);
    TabState *tabState = tabBar->tabData(arg).value<TabState*>();
    actionSaveDocument->setEnabled(nullptr != tabState && tabState->isModified); //set action enabled if new documend still modified
}

void MainWindow::OnCleanChanged(int index, bool val)
{
    DVASSERT(index >= 0);
    TabState *tabState = tabBar->tabData(index).value<TabState*>();
    tabState->isModified = !val;

    QString tabText = tabState->tabText;
    if (!val)
    {
        tabText.append('*');
    }
    tabBar->setTabText(index, tabText);

    if (index == tabBar->currentIndex())
    {
        actionSaveDocument->setEnabled(tabState->isModified);
    }
}

void MainWindow::OnOpenFontManager()
{
    FontManagerDialog fontManagerDialog(false, QString(), this);
    fontManagerDialog.exec();
}

void MainWindow::OnShowHelp()
{
    FilePath docsPath = ResourcesManageHelper::GetDocumentationPath().toStdString() + "index.html";
    QString docsFile = QString::fromStdString("file:///" + docsPath.GetAbsolutePathname());
    QDesktopServices::openUrl(QUrl(docsFile));
}

void MainWindow::InitLanguageBox()
{
    QComboBox *comboboxLanguage = new QComboBox();
    comboboxLanguage->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    QLabel *label = new QLabel(tr("language"));
    label->setBuddy(comboboxLanguage);
    QHBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->addWidget(label);
    layout->addWidget(comboboxLanguage);
    QWidget *wrapper = new QWidget();
    wrapper->setLayout(layout);
    toolBarPlugins->addWidget(wrapper);
    comboboxLanguage->setModel(localizationEditorDialog->currentLocaleComboBox->model());
    connect(comboboxLanguage, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), localizationEditorDialog->currentLocaleComboBox, &QComboBox::setCurrentIndex);
    connect(localizationEditorDialog->currentLocaleComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), comboboxLanguage, &QComboBox::setCurrentIndex);
    comboboxLanguage->setCurrentIndex(localizationEditorDialog->currentLocaleComboBox->currentIndex());
}

void MainWindow::InitMenu()
{
    SetupViewMenu();

    connect(actionSaveDocument, &QAction::triggered, this, &MainWindow::OnSaveDocument);
    connect(actionSaveAllDocuments, &QAction::triggered, this, &MainWindow::SaveAllDocuments);
    connect(actionOpen_project, &QAction::triggered, this, &MainWindow::OnOpenProject);
    connect(actionClose_project, &QAction::triggered, this, &MainWindow::CloseProject);

    connect(actionExit, &QAction::triggered, this, &MainWindow::ActionExitTriggered);
    connect(menuRecent, &QMenu::triggered, this, &MainWindow::RecentMenuTriggered);

    // Remap zoom in/out shorcuts for windows platform
#if defined(__DAVAENGINE_WIN32__)
    QList<QKeySequence> shortcuts;
    shortcuts.append(QKeySequence(Qt::CTRL + Qt::Key_Equal));
    shortcuts.append(QKeySequence(Qt::CTRL + Qt::Key_Plus));
    actionZoomIn->setShortcuts(shortcuts);
#endif

    //Help contents dialog
    connect(actionHelp, &QAction::triggered, this, &MainWindow::OnShowHelp);

    // Pixelization.
    actionPixelized->setChecked(EditorSettings::Instance()->IsPixelized());
    connect(actionPixelized, &QAction::triggered, this, &MainWindow::OnPixelizationStateChanged);
    DisableActions();
}

void MainWindow::OnSaveDocument()
{
    int index = tabBar->currentIndex();
    DVASSERT(index >= 0);
    emit SaveDocument(index);
}

void MainWindow::SetupViewMenu()
{
    // Setup the common menu actions.
    menuView->addAction(propertiesWidget->toggleViewAction());
    menuView->addAction(fileSystemDockWidget->toggleViewAction());
    menuView->addAction(packageWidget->toggleViewAction());
    menuView->addAction(libraryWidget->toggleViewAction());
    menuView->addAction(consoleDockWidget->toggleViewAction());

    menuView->addSeparator();
    menuView->addAction(mainToolbar->toggleViewAction());
    
    // Setup the Background Color menu.
    QMenu* setBackgroundColorMenu = new QMenu("Background Color", this);
    menuView->addSeparator();
    menuView->addMenu(setBackgroundColorMenu);

    static const struct
    {
        QColor color;
        QString colorName;
    } colorsMap[] =
    {
        { Qt::black, "Black" },
        { QColor(0x33, 0x33, 0x33, 0xFF), "Default" },
        { QColor(0x53, 0x53, 0x53, 0xFF), "Dark Gray" },
        { QColor(0xB8, 0xB8, 0xB8, 0xFF), "Medium Gray" },
        { QColor(0xD6, 0xD6, 0xD6, 0xFF), "Light Gray" },
    };
    
    Color curBackgroundColor = EditorSettings::Instance()->GetCurrentBackgroundFrameColor();
    int32 itemsCount = COUNT_OF(colorsMap);
    
    bool isCustomColor = true;
    for (int32 i = 0; i < itemsCount; i ++)
    {
        QAction* colorAction = new QAction(colorsMap[i].colorName, setBackgroundColorMenu);
        colorAction->setProperty(COLOR_PROPERTY_ID, colorsMap[i].color);
        
        Color curColor = QColorToColor(colorsMap[i].color);
        if (curColor == curBackgroundColor)
        {
            isCustomColor = false;
        }

        colorAction->setCheckable(true);
        colorAction->setChecked(curColor == curBackgroundColor);
        
        backgroundFramePredefinedColorActions.append(colorAction);
        setBackgroundColorMenu->addAction(colorAction);
    }
    
    backgroundFrameUseCustomColorAction = new QAction("Custom", setBackgroundColorMenu);
    backgroundFrameUseCustomColorAction->setProperty(COLOR_PROPERTY_ID, ColorToQColor(curBackgroundColor));
    backgroundFrameUseCustomColorAction->setCheckable(true);
    backgroundFrameUseCustomColorAction->setChecked(isCustomColor);
    setBackgroundColorMenu->addAction(backgroundFrameUseCustomColorAction);
    
    setBackgroundColorMenu->addSeparator();
    
    backgroundFrameSelectCustomColorAction = new QAction("Select Custom Color...", setBackgroundColorMenu);
    setBackgroundColorMenu->addAction(backgroundFrameSelectCustomColorAction);
    
    connect(setBackgroundColorMenu, SIGNAL(triggered(QAction*)), this, SLOT(SetBackgroundColorMenuTriggered(QAction*)));

    // Another actions below the Set Background Color.
    menuView->addAction(actionZoomIn);
    menuView->insertSeparator(actionZoomIn);
    menuView->addAction(actionZoomOut);
}

void MainWindow::DisableActions()
{
    actionSaveAllDocuments->setEnabled(false);
    actionSaveDocument->setEnabled(false);

    actionClose_project->setEnabled(false);
    actionFontManager->setEnabled(false);
}

void MainWindow::RebuildRecentMenu()
{
    menuRecent->clear();
    // Get up to date count of recent project actions
    int32 projectCount = EditorSettings::Instance()->GetLastOpenedCount();
    QStringList projectList;

    for (int32 i = 0; i < projectCount; ++i)
    {
        projectList << QDir::toNativeSeparators(QString(EditorSettings::Instance()->GetLastOpenedFile(i).c_str()));
    }
    projectList.removeDuplicates();
    for (auto &projectPath : projectList)
        {
            QAction *recentProject = new QAction(projectPath, this);
            recentProject->setData(projectPath);
            menuRecent->addAction(recentProject);
        }
    menuRecent->setEnabled(projectCount > 0);
}

int MainWindow::AddTab(const FilePath &scenePath)
{
    QString tabText(scenePath.GetFilename().c_str());
    int index = tabBar->addTab(tabText);
    tabBar->setTabToolTip(index, scenePath.GetAbsolutePathname().c_str());
    TabState* tabState = new TabState(tabText);
    tabBar->setTabData(index, QVariant::fromValue<TabState*>(tabState));
    OnCountChanged(tabBar->count());
    return index;
}

void MainWindow::closeEvent(QCloseEvent *ev)
{
    emit CloseRequested();
    ev->ignore();
}

void MainWindow::OnProjectOpened(const ResultList &resultList, QString projectPath)
{
    menuTools->setEnabled(resultList);
    toolBarPlugins->setEnabled(resultList);
    actionLocalizationManager->setEnabled(resultList);
    fileSystemDockWidget->setEnabled(resultList);
    if (resultList)
    {
        UpdateProjectSettings(projectPath);

        RebuildRecentMenu();
        fileSystemDockWidget->SetProjectDir(projectPath);
        localizationEditorDialog->FillLocaleComboBox();
    }
    else
    {
        QStringList errors;
        for (const auto &result : resultList.GetResults())
        {
            errors << QString::fromStdString(result.message);
        }
        QMessageBox::warning(qApp->activeWindow(), tr("Error while loading project"), errors.join('\n'));
    }
}

void MainWindow::OnOpenProject()
{
    QString projectPath = FileDialog::getOpenFileName(this, tr("Select a project file"),
                                                        ResourcesManageHelper::GetDefaultDirectory(),
                                                        tr( "Project (*.uieditor)"));
    if (projectPath.isEmpty())
    {
        return;
    }
    projectPath = QDir::toNativeSeparators(projectPath);
        
    emit ActionOpenProjectTriggered(projectPath);
}

void MainWindow::UpdateProjectSettings(const QString& projectPath)
{
    // Add file to recent project files list
    EditorSettings::Instance()->AddLastOpenedFile(projectPath.toStdString());
    
    // Save to settings default project directory
    QFileInfo fileInfo(projectPath);
    QString projectDir = fileInfo.absoluteDir().absolutePath();
    EditorSettings::Instance()->SetProjectPath(projectDir.toStdString());

	// Update window title
	this->setWindowTitle(ResourcesManageHelper::GetProjectTitle(projectPath));
    
    // Apply the pixelization value.
    Texture::SetPixelization(EditorSettings::Instance()->IsPixelized());
}

void MainWindow::OnPixelizationStateChanged()
{
    bool isPixelized = actionPixelized->isChecked();
    EditorSettings::Instance()->SetPixelized(isPixelized);

    Texture::SetPixelization(isPixelized);
}

void MainWindow::SetBackgroundColorMenuTriggered(QAction* action)
{
    Color newColor;

    if (action == backgroundFrameSelectCustomColorAction)
    {
        // Need to select new Background Frame color.
        QColor curColor = ColorToQColor(EditorSettings::Instance()->GetCustomBackgroundFrameColor());
        QColor color = QColorDialog::getColor(curColor, this, "Select color", QColorDialog::ShowAlphaChannel);
        if (color.isValid() == false)
        {
            return;
        }

        newColor = QColorToColor(color);
        EditorSettings::Instance()->SetCustomBackgroundFrameColor(newColor);
    }
    else if (action == backgroundFrameUseCustomColorAction)
    {
        // Need to use custom Background Frame Color set up earlier.
        newColor = EditorSettings::Instance()->GetCustomBackgroundFrameColor();
    }
    else
    {
        // Need to use predefined Background Frame Color.
        newColor = QColorToColor(action->property(COLOR_PROPERTY_ID).value<QColor>());
    }

    EditorSettings::Instance()->SetCurrentBackgroundFrameColor(newColor);
    
    // Update the check marks.
    bool colorFound = false;
    foreach (QAction* colorAction, backgroundFramePredefinedColorActions)
    {
        Color color = QColorToColor(colorAction->property(COLOR_PROPERTY_ID).value<QColor>());
        if (color == newColor)
        {
            colorAction->setChecked(true);
            colorFound = true;
        }
        else
        {
            colorAction->setChecked(false);
        }
    }

    // In case we don't found current color in predefined ones - select "Custom" menu item.
    backgroundFrameUseCustomColorAction->setChecked(!colorFound);
}