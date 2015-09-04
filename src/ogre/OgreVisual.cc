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
#include "ignition/rendering/ogre/OgreVisual.hh"
#include "ignition/rendering/ogre/OgreConversions.hh"
#include "ignition/rendering/ogre/OgreStorage.hh"

using namespace ignition;
using namespace rendering;

//////////////////////////////////////////////////
OgreVisual::OgreVisual()
{
}

//////////////////////////////////////////////////
OgreVisual::~OgreVisual()
{
}

//////////////////////////////////////////////////
math::Vector3d OgreVisual::GetLocalScale() const
{
  return OgreConversions::Convert(this->ogreNode->getScale());
}

//////////////////////////////////////////////////
bool OgreVisual::GetInheritScale() const
{
  return this->ogreNode->getInheritScale();
}

//////////////////////////////////////////////////
void OgreVisual::SetInheritScale(bool _inherit)
{
  this->ogreNode->setInheritScale(_inherit);
}

//////////////////////////////////////////////////
NodeStorePtr OgreVisual::GetChildren() const
{
  return this->children;
}

//////////////////////////////////////////////////
GeometryStorePtr OgreVisual::GetGeometries() const
{
  return this->geometries;
}

//////////////////////////////////////////////////
bool OgreVisual::AttachChild(NodePtr _child)
{
  OgreNodePtr derived = std::dynamic_pointer_cast<OgreNode>(_child);

  if (!derived)
  {
    gzerr << "Cannot attach node created by another render-engine" << std::endl;
    return false;
  }

  derived->SetParent(this->SharedThis());
  this->ogreNode->addChild(derived->GetOgreNode());
  return true;
}

//////////////////////////////////////////////////
bool OgreVisual::DetachChild(NodePtr _child)
{
  OgreNodePtr derived = std::dynamic_pointer_cast<OgreNode>(_child);

  if (!derived)
  {
    gzerr << "Cannot detach node created by another render-engine" << std::endl;
    return false;
  }

  this->ogreNode->removeChild(derived->GetOgreNode());
  return true;
}

//////////////////////////////////////////////////
bool OgreVisual::AttachGeometry(GeometryPtr _geometry)
{
  OgreGeometryPtr derived =
      std::dynamic_pointer_cast<OgreGeometry>(_geometry);

  if (!derived)
  {
    gzerr << "Cannot attach geometry created by another render-engine"
          << std::endl;

    return false;
  }

  derived->SetParent(this->SharedThis());
  this->ogreNode->attachObject(derived->GetOgreObject());
  return true;
}

//////////////////////////////////////////////////
bool OgreVisual::DetachGeometry(GeometryPtr _geometry)
{
  OgreGeometryPtr derived =
      std::dynamic_pointer_cast<OgreGeometry>(_geometry);

  if (!derived)
  {
    gzerr << "Cannot detach geometry created by another render-engine"
          << std::endl;

    return false;
  }

  this->ogreNode->detachObject(derived->GetOgreObject());
  return true;
}

//////////////////////////////////////////////////
void OgreVisual::SetLocalScaleImpl(const math::Vector3d &_scale)
{
  this->ogreNode->setScale(OgreConversions::Convert(_scale));
}

//////////////////////////////////////////////////
void OgreVisual::Init()
{
  BaseVisual::Init();
  this->children = OgreNodeStorePtr(new OgreNodeStore);
  this->geometries = OgreGeometryStorePtr(new OgreGeometryStore);
}

//////////////////////////////////////////////////
OgreVisualPtr OgreVisual::SharedThis()
{
  ObjectPtr object = shared_from_this();
  return std::dynamic_pointer_cast<OgreVisual>(object);
}
