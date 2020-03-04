/*
 * Copyright (C) 2015 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <ignition/common/Console.hh>

#include "ignition/rendering/ogre/OgreConversions.hh"
#include "ignition/rendering/ogre/OgreMesh.hh"
#include "ignition/rendering/ogre/OgreIncludes.hh"
#include "ignition/rendering/ogre/OgreMaterial.hh"
#include "ignition/rendering/ogre/OgreStorage.hh"
#include "ignition/rendering/ogre/OgreRTShaderSystem.hh"


/// brief Private implementation of the OgreMesh class
class ignition::rendering::OgreMeshPrivate
{
};

using namespace ignition;
using namespace rendering;

//////////////////////////////////////////////////
OgreMesh::OgreMesh()
  : dataPtr(new OgreMeshPrivate)
{
}

//////////////////////////////////////////////////
OgreMesh::~OgreMesh()
{
  this->Destroy();
}

//////////////////////////////////////////////////
void OgreMesh::Destroy()
{
  if (!this->ogreEntity)
    return;

  if (!this->Scene()->IsInitialized())
    return;

  BaseMesh::Destroy();

  auto ogreScene = std::dynamic_pointer_cast<OgreScene>(this->Scene());

  ogreScene->OgreSceneManager()->destroyEntity(this->ogreEntity);
  this->ogreEntity = nullptr;
}

//////////////////////////////////////////////////
bool OgreMesh::HasSkeleton() const
{
  return this->ogreEntity->hasSkeleton();
}

//////////////////////////////////////////////////
std::map<std::string, math::Matrix4d>
        OgreMesh::SkeletonLocalTransforms() const
{
  std::map<std::string, ignition::math::Matrix4d> mapTfs;
  if (this->ogreEntity->hasSkeleton())
  {
    Ogre::SkeletonInstance *skel = this->ogreEntity->getSkeleton();
    for (unsigned int i = 0; i < skel->getNumBones(); ++i)
    {
      Ogre::Bone *bone = skel->getBone(i);
      Ogre::Quaternion quat(bone->getOrientation());
      Ogre::Vector3 p(bone->getPosition());

      ignition::math::Quaterniond tfQuat(quat.w, quat.x, quat.y, quat.z);
      ignition::math::Vector3d tfTrans(p.x, p.y, p.z);

      ignition::math::Matrix4d tf(tfQuat);
      tf.SetTranslation(tfTrans);

      mapTfs[bone->getName()] = tf;
    }
  }

  return mapTfs;
}

//////////////////////////////////////////////////
void OgreMesh::SetSkeletonLocalTransforms(
          const std::map<std::string, math::Matrix4d> &_tfs)
{
  if (!this->ogreEntity->hasSkeleton())
  {
    return;
  }

  Ogre::SkeletonInstance *skel = this->ogreEntity->getSkeleton();

  for (auto const &[boneName, tf] : _tfs)
  {
    if (skel->hasBone(boneName))
    {
      Ogre::Bone *bone = skel->getBone(boneName);
      bone->setManuallyControlled(true);
      bone->setPosition(OgreConversions::Convert(tf.Translation()));
      bone->setOrientation(OgreConversions::Convert(tf.Rotation()));
    }
  }
}

//////////////////////////////////////////////////
void OgreMesh::SetSkeletonAnimationEnabled(const std::string &_name,
    bool _enabled, bool _loop, float _weight)
{
  if (!this->ogreEntity->hasAnimationState(_name))
  {
    ignerr << "Skeleton animation name not found: " << _name << std::endl;
    return;
  }

  // disable manual control
  if (_enabled)
  {
    Ogre::SkeletonInstance *skel = this->ogreEntity->getSkeleton();
    Ogre::Skeleton::BoneIterator iter = skel->getBoneIterator();
    while (iter.hasMoreElements())
    {
      Ogre::Bone* bone = iter.getNext();
      bone->setManuallyControlled(false);
    }
  }

  // update animation state
  Ogre::AnimationState *anim = this->ogreEntity->getAnimationState(_name);
  anim->setEnabled(_enabled);
  anim->setLoop(_loop);
  anim->setWeight(_weight);
}

//////////////////////////////////////////////////
bool OgreMesh::SkeletonAnimationEnabled(const std::string &_name) const
{
  if (!this->ogreEntity->hasAnimationState(_name))
  {
    ignerr << "Skeleton animation name not found: " << _name << std::endl;
    return false;
  }

  Ogre::AnimationState *anim = this->ogreEntity->getAnimationState(_name);
  return anim->getEnabled();
}


//////////////////////////////////////////////////
void OgreMesh::UpdateSkeletonAnimation(
    std::chrono::steady_clock::duration _time)
{
  Ogre::AnimationStateSet *animationStateSet =
      this->ogreEntity->getAllAnimationStates();

  auto it = animationStateSet->getAnimationStateIterator();
  while (it.hasMoreElements())
  {
    Ogre::AnimationState *anim = it.getNext();
    if (anim->getEnabled())
    {
      auto seconds =
          std::chrono::duration_cast<std::chrono::milliseconds>(_time).count() /
          1000.0;
      anim->setTimePosition(seconds);
    }
  }

  // this workaround is needed for ogre 1.x because we are doing manual
  // render updates.
  // see https://forums.ogre3d.org/viewtopic.php?t=33448
  Ogre::SkeletonInstance *skel = this->ogreEntity->getSkeleton();
  skel->setAnimationState(*this->ogreEntity->getAllAnimationStates());
  skel->_notifyManualBonesDirty();
}

//////////////////////////////////////////////////
Ogre::MovableObject *OgreMesh::OgreObject() const
{
  return this->ogreEntity;
}

//////////////////////////////////////////////////
SubMeshStorePtr OgreMesh::SubMeshes() const
{
  return this->subMeshes;
}

//////////////////////////////////////////////////
OgreSubMesh::OgreSubMesh()
{
}

//////////////////////////////////////////////////
OgreSubMesh::~OgreSubMesh()
{
  this->Destroy();
}

//////////////////////////////////////////////////
Ogre::SubEntity *OgreSubMesh::OgreSubEntity() const
{
  return this->ogreSubEntity;
}

//////////////////////////////////////////////////
void OgreSubMesh::Destroy()
{
  OgreRTShaderSystem::Instance()->DetachEntity(this);

  BaseSubMesh::Destroy();
}

//////////////////////////////////////////////////
void OgreSubMesh::SetMaterialImpl(MaterialPtr _material)
{
  OgreMaterialPtr derived =
      std::dynamic_pointer_cast<OgreMaterial>(_material);

  if (!derived)
  {
    ignerr << "Cannot assign material created by another render-engine"
        << std::endl;

    return;
  }

  std::string materialName = derived->Name();
  Ogre::MaterialPtr ogreMaterial = derived->Material();
  this->ogreSubEntity->setMaterialName(materialName);

  // set cast shadows
  this->ogreSubEntity->getParent()->setCastShadows(_material->CastShadows());
}

//////////////////////////////////////////////////
void OgreSubMesh::Init()
{
  BaseSubMesh::Init();
  OgreRTShaderSystem::Instance()->AttachEntity(this);
}

//////////////////////////////////////////////////
