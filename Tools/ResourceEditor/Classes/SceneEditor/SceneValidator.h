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



#ifndef __SCENE_VALIDATOR_H__
#define __SCENE_VALIDATOR_H__

#include "DAVAEngine.h"

using namespace DAVA;

class SceneValidator: public Singleton<SceneValidator>
{
    
public:
    SceneValidator();
    virtual ~SceneValidator();

    /*
     \brief Function to validate Scene errors and Displays errors log at Errors Dialog
     \param[in] scene scene for validation
     \returns true if errors were found
	 */
    bool ValidateSceneAndShowErrors(Scene *scene);
    
	/*
     \brief Function to validate Particle Emitter upon loading.
     \param[in] emitter Particle Emitter to validate
     \param[out] errorsLog set for validation erros
	 */
	bool ValidateParticleEmitter(ParticleEmitter* emitter, Set<String> &errorsLog);
	
    /*
     \brief Function to validate Scene errors
     \param[in] scene scene for validation
     \param[out] errorsLog set for validation erros
	 */
    void ValidateScene(Scene *scene, Set<String> &errorsLog);
    
    /*
     \brief Function to find Scales in models transformations
     \param[in] scene scene for validation
     \param[out] errorsLog set for validation erros
	 */
	void ValidateScales(Scene *scene, Set<String> &errorsLog);


    /*
     \brief Function to validate Texture errors
     
     Displays errors log at Errors Dialog
     
     \param[in] texture texture for validation
	 */
    void ValidateTextureAndShowErrors(Texture *texture, const FilePath &textureName, const String &validatedObjectName);

    /*
     \brief Function to validate Texture errors
     \param[in] texture texture for validation
     \param[out] errorsLog set for validation erros
	 */
    
    void ValidateTexture(Texture *texture, const FilePath &texturePathname, const String &validatedObjectName, Set<String> &errorsLog);


    /*
     \brief Function to validate LandscapeNode errors
     \param[in] landscape landscape for validation
     \param[out] errorsLog set for validation erros
	 */
    void ValidateLandscape(Landscape *landscape, Set<String> &errorsLog);
        
    /*
     \brief Function to validate Entity errors
     \param[in] sceneNode sceneNode for validation
     \param[out] errorsLog set for validation erros
	 */
    void ValidateSceneNode(Entity *sceneNode, Set<String> &errorsLog);
    
    /*
     \brief Function to validate Material errors
     \param[in] material material for validation
     \param[out] errorsLog set for validation erros
	 */
    void ValidateMaterial(Material *material, Set<String> &errorsLog);

    /*
     \brief Function sets 3d folder path for cheking texture pathnames
     \param[in] pathname path to DataSource/3d folder
     \return old path for checking
	 */
    FilePath SetPathForChecking(const FilePath &pathname);
    
//    void EnumerateSceneTextures();
    void EnumerateNodes(Scene *scene);
    
    static bool IsTextureChanged(const TextureDescriptor *descriptor, eGPUFamily forGPU);
    static bool IsTextureChanged(const FilePath &texturePathname, eGPUFamily forGPU);
    
	bool ValidateTexturePathname(const FilePath &pathForValidation, Set<String> &errorsLog);
	bool ValidateHeightmapPathname(const FilePath &pathForValidation, Set<String> &errorsLog);

    bool IsPathCorrectForProject(const FilePath &pathname);
    
    
protected:

    void ValidateRenderComponent(Entity *ownerNode, Set<String> &errorsLog);
    void ValidateLodComponent(Entity *ownerNode, Set<String> &errorsLog);
    void ValidateParticleEmitterComponent(Entity *ownerNode, Set<String> &errorsLog);

    void ValidateRenderBatch(Entity *ownerNode, RenderBatch *renderBatch, Set<String> &errorsLog);
    void ValidateInstanceMaterialState(InstanceMaterialState *materialState, Set<String> &errorsLog);

    
	void ValidateLandscapeTexture(Landscape *landscape, Landscape::eTextureLevel texLevel, Set<String> &errorsLog);

    
    int32 EnumerateSceneNodes(Entity *node);
    
	void ValidateScalesInternal(Entity *sceneNode, Set<String> &errorsLog);
    
    bool ValidatePathname(const FilePath &pathForValidation, const String &validatedObjectName);

    bool NodeRemovingDisabled(Entity *node);
    
    bool WasTextureChanged(Texture *texture, eGPUFamily forGPU);

	bool IsTextureDescriptorPath(const FilePath &path);
    
    bool IsFBOTexture(Texture *texture);
    
	DAVA_DEPRECATED(void ConvertLightmapSizeFromProperty(Entity *ownerNode, InstanceMaterialState *materialState));

    Set<Entity *> emptyNodesForDeletion;
    Set<String> errorMessages;
    
//    int32 sceneTextureCount;
//    int32 sceneTextureMemory;
    
    FilePath pathForChecking;
};



#endif // __SCENE_VALIDATOR_H__