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



#include "CustomColorsProxy.h"
#include "Deprecated/EditorConfig.h"

CustomColorsProxy::CustomColorsProxy(int32 size)
    : changedRect(Rect())
    , spriteChanged(false)
    , textureLoaded(false)
    , size(size)
    , changes(0)
{
	customColorsRenderTarget = Texture::CreateFBO(size, size, FORMAT_RGBA8888/*, Texture::DEPTH_NONE*/);
	UpdateSpriteFromConfig();
}

void CustomColorsProxy::ResetLoadedState( bool isLoaded )
{
    textureLoaded = isLoaded;
}

bool CustomColorsProxy::IsTextureLoaded() const
{
    return textureLoaded;
}

CustomColorsProxy::~CustomColorsProxy()
{
    SafeRelease(customColorsRenderTarget);
}

Texture* CustomColorsProxy::GetTexture()
{
    return customColorsRenderTarget;
}

void CustomColorsProxy::ResetTargetChanged()
{
	spriteChanged = false;
}

bool CustomColorsProxy::IsTargetChanged()
{
	return spriteChanged;
}

Rect CustomColorsProxy::GetChangedRect()
{
	if (IsTargetChanged())
	{
		return changedRect;
	}
	
	return Rect();
}

void CustomColorsProxy::UpdateRect(const DAVA::Rect &rect)
{
	DAVA::Rect bounds(0.f, 0.f, (float32)size, (float32)size);
	changedRect = rect;
	bounds.ClampToRect(changedRect);

	spriteChanged = true;
}

int32 CustomColorsProxy::GetChangesCount() const
{
	return changes;
}

void CustomColorsProxy::ResetChanges()
{
	changes = 0;
}

void CustomColorsProxy::IncrementChanges()
{
	++changes;
}

void CustomColorsProxy::DecrementChanges()
{
	--changes;
}

void CustomColorsProxy::UpdateSpriteFromConfig()
{
    if (NULL == customColorsRenderTarget)
	{
		return;
	}
#if RHI_COMPLETE_EDITOR
    RenderManager::Instance()->SetRenderTarget(customColorsRenderTarget);
    RenderManager::Instance()->SetViewport(Rect(0.f, 0.f, (float32)customColorsRenderTarget->GetWidth(), customColorsRenderTarget->GetHeight()));
	Vector<Color> customColors = EditorConfig::Instance()->GetColorPropertyValues("LandscapeCustomColors");
	if (customColors.size())
	{
		Color color = customColors.front();
		RenderManager::Instance()->ClearWithColor(color.r, color.g, color.b, color.a);
	}
    RenderManager::Instance()->SetRenderTarget(0);
#endif // RHI_COMPLETE_EDITOR
}
