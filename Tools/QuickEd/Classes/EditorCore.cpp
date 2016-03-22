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


#include "Platform/Qt5/QtLayer.h"
#include "UI/mainwindow.h"
#include "UI/Preview/ScrollAreaController.h"
#include "UI/Preview/Ruler/RulerController.h"
#include "DocumentGroup.h"
#include "Document.h"
#include "EditorCore.h"
#include "Model/PackageHierarchy/PackageNode.h"
#include "QtTools/ReloadSprites/DialogReloadSprites.h"
#include "QtTools/ReloadSprites/SpritesPacker.h"
#include "QtTools/DavaGLWidget/davaglwidget.h"
#include "EditorSettings.h"

#include <QSettings>
#include <QVariant>
#include <QByteArray>
#include <QFileSystemWatcher>

#include "UI/Layouts/UILayoutSystem.h"
#include "UI/Styles/UIStyleSheetSystem.h"
#include "UI/UIControlSystem.h"
#include "Utils/Utils.h"
#include "UI/FileSystemView/FileSystemModel.h"
#include "UI/Package/PackageModel.h"

using namespace DAVA;

EditorCore::EditorCore(QObject* parent)
    : QObject(parent)
    , Singleton<EditorCore>()
    , spritesPacker(std::make_unique<SpritesPacker>())
    , project(new Project(this))
    , documentGroup(new DocumentGroup(project, this))
    , mainWindow(std::make_unique<MainWindow>())
{
    mainWindow->setWindowIcon(QIcon(":/icon.ico"));
    mainWindow->AttachDocumentGroup(documentGroup);

    connect(mainWindow->actionClose_project, &QAction::triggered, this, &EditorCore::CloseProject);
    connect(mainWindow->actionReloadSprites, &QAction::triggered, this, &EditorCore::OnReloadSprites);
    connect(project, &Project::IsOpenChanged, mainWindow->actionClose_project, &QAction::setEnabled);
    connect(project, &Project::ProjectPathChanged, this, &EditorCore::OnProjectPathChanged);
    connect(project, &Project::ProjectPathChanged, mainWindow->fileSystemDockWidget, &FileSystemDockWidget::SetProjectDir);
    connect(project, &Project::IsOpenChanged, mainWindow->fileSystemDockWidget, &FileSystemDockWidget::setEnabled);

    connect(mainWindow.get(), &MainWindow::CloseProject, this, &EditorCore::CloseProject);
    connect(mainWindow.get(), &MainWindow::ActionExitTriggered, this, &EditorCore::OnExit);
    connect(mainWindow.get(), &MainWindow::CloseRequested, this, &EditorCore::CloseProject);
    connect(mainWindow.get(), &MainWindow::RecentMenuTriggered, this, &EditorCore::RecentMenu);
    connect(mainWindow.get(), &MainWindow::ActionOpenProjectTriggered, this, &EditorCore::OpenProject);
    connect(mainWindow.get(), &MainWindow::OpenPackageFile, documentGroup, &DocumentGroup::AddDocument);
    connect(mainWindow.get(), &MainWindow::RtlChanged, this, &EditorCore::OnRtlChanged);
    connect(mainWindow.get(), &MainWindow::BiDiSupportChanged, this, &EditorCore::OnBiDiSupportChanged);
    connect(mainWindow.get(), &MainWindow::GlobalStyleClassesChanged, this, &EditorCore::OnGlobalStyleClassesChanged);

    QComboBox* languageComboBox = mainWindow->GetComboBoxLanguage();
    EditorLocalizationSystem* editorLocalizationSystem = project->GetEditorLocalizationSystem();
    connect(languageComboBox, &QComboBox::currentTextChanged, editorLocalizationSystem, &EditorLocalizationSystem::SetCurrentLocale);
    connect(editorLocalizationSystem, &EditorLocalizationSystem::CurrentLocaleChanged, languageComboBox, &QComboBox::setCurrentText);

    auto previewWidget = mainWindow->previewWidget;

    connect(documentGroup, &DocumentGroup::ActiveDocumentChanged, previewWidget, &PreviewWidget::SaveSystemsContextAndClear); //this context will affect other widgets, so he must be updated before other widgets take new document
    connect(documentGroup, &DocumentGroup::ActiveDocumentChanged, previewWidget, &PreviewWidget::OnDocumentChanged);
    connect(mainWindow.get(), &MainWindow::EmulationModeChanged, previewWidget, &PreviewWidget::OnEmulationModeChanged);

    connect(documentGroup, &DocumentGroup::ActiveDocumentChanged, mainWindow.get(), &MainWindow::OnDocumentChanged);
    connect(documentGroup, &DocumentGroup::ActiveDocumentChanged, mainWindow->libraryWidget, &LibraryWidget::OnDocumentChanged);

    connect(documentGroup, &DocumentGroup::ActiveDocumentChanged, mainWindow->propertiesWidget, &PropertiesWidget::OnDocumentChanged);

    connect(previewWidget, &PreviewWidget::OpenPackageFile, this, &EditorCore::OnOpenPackageFile);
    auto packageWidget = mainWindow->packageWidget;
    connect(previewWidget, &PreviewWidget::DropRequested, packageWidget->GetPackageModel(), &PackageModel::OnDropMimeData, Qt::DirectConnection);
    connect(documentGroup, &DocumentGroup::ActiveDocumentChanged, packageWidget, &PackageWidget::OnDocumentChanged);
    connect(previewWidget, &PreviewWidget::DeleteRequested, packageWidget, &PackageWidget::OnDelete);
    connect(previewWidget, &PreviewWidget::ImportRequested, packageWidget, &PackageWidget::OnImport);
    connect(previewWidget, &PreviewWidget::CutRequested, packageWidget, &PackageWidget::OnCut);
    connect(previewWidget, &PreviewWidget::CopyRequested, packageWidget, &PackageWidget::OnCopy);
    connect(previewWidget, &PreviewWidget::PasteRequested, packageWidget, &PackageWidget::OnPaste);
    connect(previewWidget, &PreviewWidget::SelectionChanged, packageWidget, &PackageWidget::OnSelectionChanged);
    connect(packageWidget, &PackageWidget::SelectedNodesChanged, previewWidget, &PreviewWidget::OnSelectionChanged);
    connect(packageWidget, &PackageWidget::CurrentIndexChanged, mainWindow->propertiesWidget, &PropertiesWidget::UpdateModel);

    connect(previewWidget->GetGLWidget(), &DavaGLWidget::Initialized, this, &EditorCore::OnGLWidgedInitialized);
    connect(project->GetEditorLocalizationSystem(), &EditorLocalizationSystem::CurrentLocaleChanged, this, &EditorCore::UpdateLanguage);

    connect(documentGroup, &DocumentGroup::ActiveDocumentChanged, previewWidget, &PreviewWidget::LoadSystemsContext); //this context will affect other widgets, so he must be updated when other widgets took new document

    connect(spritesPacker.get(), &SpritesPacker::Finished, []()
            {
                Sprite::ReloadSprites();
            });
}

EditorCore::~EditorCore() = default;

MainWindow* EditorCore::GetMainWindow() const
{
    return mainWindow.get();
}

Project* EditorCore::GetProject() const
{
    return project;
}

void EditorCore::Start()
{
    mainWindow->show();
}

void EditorCore::OnReloadSprites()
{
    for (auto& document : documentGroup->GetDocuments())
    {
        if (!documentGroup->RemoveDocument(document))
        {
            return;
        }
    }
    mainWindow->ExecDialogReloadSprites(spritesPacker.get());
}

void EditorCore::OnGLWidgedInitialized()
{
    int32 projectCount = EditorSettings::Instance()->GetLastOpenedCount();
    if (projectCount > 0)
    {
        OpenProject(QDir::toNativeSeparators(QString(EditorSettings::Instance()->GetLastOpenedFile(0).c_str())));
    }
}

void EditorCore::OnProjectPathChanged(const QString& projectPath)
{
    if (EditorSettings::Instance()->IsUsingAssetCache())
    {
        spritesPacker->SetCacheTool(
        EditorSettings::Instance()->GetAssetCacheIp(),
        EditorSettings::Instance()->GetAssetCachePort(),
        EditorSettings::Instance()->GetAssetCacheTimeoutSec());
    }
    else
    {
        spritesPacker->ClearCacheTool();
    }

    QRegularExpression searchOption("gfx\\d*$", QRegularExpression::CaseInsensitiveOption);
    spritesPacker->ClearTasks();
    QDirIterator it(projectPath + "/DataSource");
    while (it.hasNext())
    {
        const QFileInfo& fileInfo = it.fileInfo();
        it.next();
        if (fileInfo.isDir())
        {
            QString outputPath = fileInfo.absoluteFilePath();
            if (!outputPath.contains(searchOption))
            {
                continue;
            }
            outputPath.replace(outputPath.lastIndexOf("DataSource"), QString("DataSource").size(), "Data");
            QDir outputDir(outputPath);
            spritesPacker->AddTask(fileInfo.absoluteFilePath(), outputDir);
        }
    }
}

void EditorCore::RecentMenu(QAction* recentProjectAction)
{
    QString projectPath = recentProjectAction->data().toString();

    if (projectPath.isEmpty())
    {
        return;
    }
    OpenProject(projectPath);
}

void EditorCore::UpdateLanguage()
{
    project->GetEditorFontSystem()->RegisterCurrentLocaleFonts();
    for (auto& document : documentGroup->GetDocuments())
    {
        document->RefreshAllControlProperties();
        document->RefreshLayout();
    }
}

void EditorCore::OnRtlChanged(bool isRtl)
{
    UIControlSystem::Instance()->SetRtl(isRtl);
    for (auto& document : documentGroup->GetDocuments())
    {
        document->RefreshAllControlProperties();
        document->RefreshLayout();
    }
}

void EditorCore::OnBiDiSupportChanged(bool support)
{
    UIControlSystem::Instance()->SetBiDiSupportEnabled(support);
    for (auto& document : documentGroup->GetDocuments())
    {
        document->RefreshAllControlProperties();
        document->RefreshLayout();
    }
}

void EditorCore::OnGlobalStyleClassesChanged(const QString& classesStr)
{
    Vector<String> tokens;
    Split(classesStr.toStdString(), " ", tokens);

    UIControlSystem::Instance()->GetStyleSheetSystem()->ClearGlobalClasses();
    for (String& token : tokens)
        UIControlSystem::Instance()->GetStyleSheetSystem()->AddGlobalClass(FastName(token));

    for (auto& document : documentGroup->GetDocuments())
    {
        document->RefreshAllControlProperties();
        document->RefreshLayout();
    }
}

void EditorCore::OpenProject(const QString& path)
{
    if (!CloseProject())
    {
        return;
    }

    if (!project->CheckAndUnlockProject(path))
    {
        return;
    }
    ResultList resultList;
    if (!project->Open(path))
    {
        resultList.AddResult(Result::RESULT_ERROR, "Error while loading project");
    }
    mainWindow->OnProjectOpened(resultList, project);
}

bool EditorCore::CloseProject()
{
    if (!project->IsOpen())
    {
        return true;
    }
    auto documents = documentGroup->GetDocuments();
    bool hasUnsaved = std::find_if(documents.begin(), documents.end(), [](Document* document) { return document->CanSave(); }) != documents.end();

    if (hasUnsaved)
    {
        int ret = QMessageBox::question(
        qApp->activeWindow(),
        tr("Save changes"),
        tr("Some files has been modified.\n"
           "Do you want to save your changes?"),
        QMessageBox::SaveAll | QMessageBox::NoToAll | QMessageBox::Cancel);
        if (ret == QMessageBox::Cancel)
        {
            return false;
        }
        else if (ret == QMessageBox::SaveAll)
        {
            documentGroup->SaveAllDocuments();
        }
    }

    for (auto& document : documentGroup->GetDocuments())
    {
        if (!documentGroup->RemoveDocument(document))
        {
            DVASSERT(false && "can not close saved documents");
            return false;
        }
    }
    project->Close();
    return true;
}

void EditorCore::OnExit()
{
    if (CloseProject())
    {
        qApp->quit();
    }
}
