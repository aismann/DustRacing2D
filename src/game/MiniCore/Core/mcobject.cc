// This file belongs to the "MiniCore" game engine.
// Copyright (C) 2010 Jussi Lind <jussi.lind@iki.fi>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
// MA  02110-1301, USA.
//

#include "mcobject.hh"
#include "mcobjectimpl.hh"
#include "mcshape.hh"
#include "mcshapeview.hh"
#include "mccircleshape.hh"
#include "mcrectshape.hh"
#include "mcsurfaceview.hh"
#include "mcsurface.hh"
#include "mcworld.hh"
#include "mcevent.hh"
#include "mccollisionevent.hh"
#include "mcoutofboundariesevent.hh"
#include "mccamera.hh"
#include "mctrigonom.hh"

MCUint MCObjectImpl::typeIDCount = 1;
MCObjectImpl::TypeHash MCObjectImpl::typeHash;

namespace
{
enum PropertyMask
{
    // Object is stationary
    StationaryMask = (1<<0),

    // Object is renderable
    RenderableMask = (1<<1),

    // Object is considered in physics calculations
    PhysicsMask = (1<<2),

    // Object is considered in collision calculations
    CollisionsMask = (1<<3),

    // Object has a shadow
    ShadowMask = (1<<4),

    // Object is scheduled to be removed from the world
    RemovingMask = (1<<5),

    // View renders outline for debug
    OutlineMask = (1<<6)
};
// Physics damping factor
const MCFloat DampingFactor = 0.999;
}

MCObjectImpl::MCObjectImpl(MCObject * pPublic, const std::string & typeId)
: pPublic(pPublic)
, typeID(registerType(typeId))
, time(0)
, invMass(std::numeric_limits<MCFloat>::max())
, mass(0)
, restitution(0.5f)
, xyFriction(0.0f)
, angle(0)
, angularAcceleration(0.0f)
, angularVelocity(0.0f)
, maximumAngularVelocity(-1)
, maximumVelocity(-1)
, moment(0.0f)
, layer(0)
, index(-1)
, flags(RenderableMask | PhysicsMask | CollisionsMask | ShadowMask)
, i0(0), i1(0), j0(0), j1(0)
, pShape(nullptr)
{}

void MCObjectImpl::setFlag(MCUint flag, bool enable)
{
    flags = enable ? flags | flag : flags & ~flag;
}

MCUint MCObjectImpl::getTypeIDForName(const std::string & typeName)
{
    auto i(typeHash.find(typeName));
    return i == typeHash.end() ? 0 : i->second;
}

MCUint MCObjectImpl::registerType(const std::string & typeName)
{
    auto i(typeHash.find(typeName));
    if (i == typeHash.end()) {
        typeIDCount++;
        typeHash[typeName] = typeIDCount;
        return typeIDCount;
    } else {
        return i->second;
    }
}

void MCObjectImpl::integrate(MCFloat step)
{
    if (step > 0.0) {

        MCVector3dF totAcceleration(acceleration);
        pPublic->translate(location + velocity * step);

        totAcceleration += forces * invMass;
        velocity        += totAcceleration * step;
        velocity        *= DampingFactor;

        // Note that this code doesn't take the z-component into consideration
        if (maximumVelocity > 0) {
            const MCFloat l = MCVector2dF(velocity).lengthFast();
            if (l > maximumVelocity) {
                velocity /= l;
                velocity *= maximumVelocity;
            }
        }

        if (pShape)
        {
            if (pShape->momentOfInertia() > 0.0f)
            {
                MCFloat totAngularAcceleration(angularAcceleration);
                const MCFloat newAngle = angle + MCTrigonom::radToDeg(angularVelocity * step);
                doRotate(newAngle);
                angle = newAngle;

                totAngularAcceleration += moment / pShape->momentOfInertia();
                angularVelocity        += totAngularAcceleration * step;
                angularVelocity        *= DampingFactor;

                if (maximumAngularVelocity > 0) {
                    if (angularVelocity > 0) {
                        if (angularVelocity > maximumAngularVelocity) {
                            angularVelocity = maximumAngularVelocity;
                        }
                    }
                    else if (-angularVelocity > maximumAngularVelocity) {
                        angularVelocity = -maximumAngularVelocity;
                    }
                }
            }
        }

        forces.setZero();
        moment = 0.0f;
        doOutOfBoundariesEvent();
    }
}

void MCObjectImpl::doOutOfBoundariesEvent()
{
    // By default use the center point as the test point.
    MCFloat minX = location.i();
    MCFloat maxX = location.i();
    MCFloat minY = location.j();
    MCFloat maxY = location.j();

    // Use shape bbox if shape is defined.
    if (pShape)
    {
        minX = pShape->bbox().x1();
        maxX = pShape->bbox().x2();
        minY = pShape->bbox().y1();
        maxY = pShape->bbox().y2();
    }

    // Check X-boundaries
    const MCWorld * pWorld = &MCWorld::instance();
    if (minX < pWorld->minX()) {
        velocity.setI(0); forces.setI(0);
        pPublic->translate(MCVector3dF(
            location.i() + pWorld->minX() - minX, location.j(), location.k()));
        MCOutOfBoundariesEvent e(MCOutOfBoundariesEvent::West);
        pPublic->outOfBoundariesEvent(e);
    } else if (maxX > pWorld->maxX()) {
        velocity.setI(0); forces.setI(0);
        pPublic->translate(MCVector3dF(
            location.i() + pWorld->maxX() - maxX, location.j(), location.k()));
        MCOutOfBoundariesEvent e(MCOutOfBoundariesEvent::East);
        pPublic->outOfBoundariesEvent(e);
    }

    // Check Y-boundaries
    if (minY < pWorld->minY()) {
        velocity.setJ(0); forces.setJ(0);
        pPublic->translate(MCVector3dF(
            location.i(), location.j() + pWorld->minY() - minY, location.k()));
        MCOutOfBoundariesEvent e(MCOutOfBoundariesEvent::South);
        pPublic->outOfBoundariesEvent(e);
    } else if (maxY > pWorld->maxY()) {
        velocity.setJ(0); forces.setJ(0);
        pPublic->translate(MCVector3dF(
            location.i(), location.j() + pWorld->maxY() - maxY, location.k()));
        MCOutOfBoundariesEvent e(MCOutOfBoundariesEvent::North);
        pPublic->outOfBoundariesEvent(e);
    }

    // Check Z-boundaries
    if (location.k() < pWorld->minZ()) {
        velocity.setK(0); forces.setK(0);
        pPublic->translate(
            MCVector3dF(location.i(), location.j(), pWorld->minZ()));
        MCOutOfBoundariesEvent e(MCOutOfBoundariesEvent::Bottom);
        pPublic->outOfBoundariesEvent(e);
    } else if (location.k() > pWorld->maxZ()) {
        velocity.setK(0); forces.setK(0);
        pPublic->translate(
            MCVector3dF(location.i(), location.j(), pWorld->maxZ()));
        MCOutOfBoundariesEvent e(MCOutOfBoundariesEvent::Top);
        pPublic->outOfBoundariesEvent(e);
    }
}

MCObjectImpl::~MCObjectImpl()
{}

MCObject::MCObject(const std::string & typeId)
: m_pImpl(new MCObjectImpl(this, typeId))
{}

MCObject::MCObject(MCShape * pShape, const std::string & typeId)
: m_pImpl(new MCObjectImpl(this, typeId))
{
    m_pImpl->pShape = pShape;
}

MCObject::MCObject(MCSurface * pSurface, const std::string & typeId)
 : m_pImpl(new MCObjectImpl(this, typeId))
{
    // Create an MCRectShape using pSurface with an MCSurfaceView
    MCRectShape * rectShape = new MCRectShape(*this,
        new MCSurfaceView(pSurface),
        pSurface ? pSurface->width() : 0,
        pSurface ? pSurface->height() : 0);
    m_pImpl->pShape = rectShape;
}

void MCObject::resetMotion()
{
    m_pImpl->velocity.setZero();
    m_pImpl->forces.setZero();
    m_pImpl->moment = 0.0f;
}

void MCObject::setSurface(MCSurface * pSurface)
{
    if (m_pImpl->pShape && m_pImpl->pShape->view()) {
        MCSurfaceView * pView = dynamic_cast<MCSurfaceView *>(
            m_pImpl->pShape->view());
        if (pView) {
            if (pView->surface() != pSurface) {
                pView->setSurface(pSurface);
            }
        }
    }
}

MCSurface * MCObject::surface() const
{
    if (m_pImpl->pShape && m_pImpl->pShape->view()) {
        MCSurfaceView * pView = dynamic_cast<MCSurfaceView *>(
            m_pImpl->pShape->view());
        if (pView) {
            return pView->surface();
        }
    }
    return nullptr;
}

MCUint MCObject::typeID() const
{
    return m_pImpl->typeID;
}

MCUint MCObject::typeID(const std::string & typeName)
{
    return MCObjectImpl::getTypeIDForName(typeName);
}

MCUint MCObject::registerType(const std::string & typeName)
{
    return MCObjectImpl::registerType(typeName);
}

bool MCObject::event(MCEvent & event)
{
    if (event.instanceTypeID() == MCCollisionEvent::typeID())
    {
        collisionEvent(static_cast<MCCollisionEvent &>(event));
        return true;
    }
    else if (event.instanceTypeID() == MCOutOfBoundariesEvent::typeID()) {
        outOfBoundariesEvent(static_cast<MCOutOfBoundariesEvent &>(event));
        return true;
    }

    return false;
}

void MCObject::collisionEvent(MCCollisionEvent & event)
{
    event.accept();
}

void MCObject::outOfBoundariesEvent(MCOutOfBoundariesEvent & event)
{
    event.accept();
}

void MCObject::sendEvent(MCObject & object, MCEvent & event)
{
    object.event(event);
}

void MCObject::addToWorld()
{
    MCWorld::instance().addObject(*this);
}

void MCObject::addToWorld(MCFloat x, MCFloat y, MCFloat z)
{
    MCWorld::instance().addObject(*this);
    translate(MCVector3dF(x, y, z));
}

void MCObject::removeFromWorld()
{
    MCWorld::instance().removeObject(*this);
}

void MCObject::removeFromWorldNow()
{
    MCWorld::instance().removeObjectNow(*this);
}

void MCObject::render(MCCamera * p)
{
    if (m_pImpl->pShape) {
        m_pImpl->pShape->render(p);
    }
}

void MCObject::renderShadow(MCCamera * p)
{
    if (m_pImpl->pShape) {
        m_pImpl->pShape->renderShadow(p);
    }
}

void MCObject::setMass(MCFloat newMass, bool stationary_)
{
    m_pImpl->setFlag(StationaryMask, stationary_);

    if (!stationary_) {
        if (newMass > 0) {
            m_pImpl->invMass = 1.0 / newMass;
        }
        else {
            m_pImpl->invMass = std::numeric_limits<MCFloat>::max();
        }
        m_pImpl->mass = newMass;
    }
    else {
        m_pImpl->invMass = 0;
        m_pImpl->mass    = std::numeric_limits<MCFloat>::max();
    }
}

MCFloat MCObject::invMass() const
{
    return m_pImpl->invMass;
}

MCFloat MCObject::mass() const
{
    return m_pImpl->mass;
}

bool MCObject::stationary() const
{
    return m_pImpl->flags & StationaryMask;
}

void MCObject::addImpulse(const MCVector3dF & impulse)
{
    m_pImpl->velocity += impulse;
}

void MCObject::setPhysicsObject(bool flag)
{
    m_pImpl->setFlag(PhysicsMask, flag);
}

bool MCObject::physicsObject() const
{
    return m_pImpl->flags & PhysicsMask;
}

void MCObject::setBypassCollisions(bool flag)
{
    m_pImpl->setFlag(CollisionsMask, !flag);
}

bool MCObject::bypassCollisions() const
{
    return !(m_pImpl->flags & CollisionsMask);
}

void MCObject::setRenderable(bool flag)
{
    m_pImpl->setFlag(RenderableMask, flag);
}

bool MCObject::renderable() const
{
    return m_pImpl->flags & RenderableMask;
}

void MCObject::setRenderShapeOutline(bool flag)
{
    m_pImpl->setFlag(OutlineMask, flag);
}

bool MCObject::renderShapeOutline() const
{
    return m_pImpl->flags & OutlineMask;
}

void MCObject::setHasShadow(bool flag)
{
    m_pImpl->setFlag(ShadowMask, flag);
}

bool MCObject::hasShadow() const
{
    return m_pImpl->flags & ShadowMask;
}

void MCObject::setMaximumVelocity(MCFloat maxVelocity)
{
    m_pImpl->maximumVelocity = maxVelocity;
}

void MCObject::setVelocity(const MCVector3dF & newVelocity)
{
    m_pImpl->velocity = newVelocity;
}

const MCVector3dF & MCObject::velocity() const
{
    return m_pImpl->velocity;
}

void MCObject::setAngularVelocity(MCFloat newVelocity)
{
    m_pImpl->angularVelocity = newVelocity;
}

MCFloat MCObject::angularVelocity() const
{
    return m_pImpl->angularVelocity;
}

void MCObject::setMaximumAngularVelocity(MCFloat newVelocity)
{
    m_pImpl->maximumAngularVelocity = newVelocity;
}

void MCObject::setAcceleration(const MCVector3dF & newAcceleration)
{
    m_pImpl->acceleration = newAcceleration;
}

const MCVector3dF & MCObject::acceleration() const
{
    return m_pImpl->acceleration;
}

void MCObject::translate(const MCVector3dF & newLocation)
{
    const bool wasInWorld = !removing() &&
        MCWorld::instance().objectTree().remove(*this);
    m_pImpl->location = newLocation;
    if (m_pImpl->pShape) {
        m_pImpl->pShape->translate(newLocation);
    }
    if (wasInWorld) {
        MCWorld::instance().objectTree().insert(*this);
    }
}

void MCObject::displace(const MCVector3dF & displacement)
{
    translate(m_pImpl->location + displacement);
}

const MCVector3dF & MCObject::location() const
{
    return m_pImpl->location;
}

void MCObject::setShadowOffset(const MCVector2dF & p)
{
    if (m_pImpl->pShape) {
        m_pImpl->pShape->setShadowOffset(p);
    }
}

MCFloat MCObject::getX() const
{
    return m_pImpl->location.i();
}

MCFloat MCObject::getY() const
{
    return m_pImpl->location.j();
}

MCFloat MCObject::getZ() const
{
    return m_pImpl->location.k();
}

void MCObject::setCenterOfRotation(MCVector2dF center)
{
    m_pImpl->centerOfRotation = center - MCVector2dF(location());
}

void MCObject::rotate(MCFloat newAngle)
{ 
    m_pImpl->rotate(newAngle);
}

void MCObjectImpl::rotate(MCFloat newAngle)
{
    doRotate(newAngle);
    angle = newAngle;
}

void MCObjectImpl::doRotate(MCFloat newAngle)
{
    if (newAngle != angle)
    {
        if (!centerOfRotation.isZero())
        {
            MCVector2dF centerOfRotation1;
            MCTrigonom::rotated(centerOfRotation, centerOfRotation1, newAngle - angle);
            pPublic->displace(centerOfRotation - centerOfRotation1);
        }

        if (pShape)
        {
            if (pShape->instanceTypeID() == MCCircleShape::typeID())
            {
                pShape->rotate(newAngle);
            }
            else
            {
                const bool wasInWorld = MCWorld::instance().objectTree().remove(*pPublic);

                pShape->rotate(newAngle);

                if (wasInWorld)
                {
                    MCWorld::instance().objectTree().insert(*pPublic);
                }
            }
        }
    }
}

MCFloat MCObject::angle() const
{
    return m_pImpl->angle;
}

MCVector2dF MCObject::direction() const
{
    return MCVector2dF(MCTrigonom::cos(angle()), MCTrigonom::sin(angle()));
}

void MCObject::setRestitution(MCFloat newRestitution)
{
    newRestitution = newRestitution < 0.0 ? 0.0 : newRestitution;
    newRestitution = newRestitution > 1.0 ? 1.0 : newRestitution;
    m_pImpl->restitution = newRestitution;
}

MCFloat MCObject::restitution() const
{
    return m_pImpl->restitution;
}

void MCObject::setShape(MCShape * newShape)
{
    if (m_pImpl->pShape) {
        delete m_pImpl->pShape;
    }

    m_pImpl->pShape = newShape;
}

MCShape * MCObject::shape() const
{
    return m_pImpl->pShape;
}

void MCObject::setView(MCShapeView * newView)
{
    if (m_pImpl->pShape) {
        m_pImpl->pShape->setView(newView);
    }
}

MCShapeView * MCObject::view() const
{
    return m_pImpl->pShape ? m_pImpl->pShape->view() : nullptr;
}

void MCObject::addForce(const MCVector3dF & force)
{
    m_pImpl->forces += force;
}

void MCObject::addMoment(MCFloat moment)
{
    m_pImpl->moment += moment;
}

void MCObject::clearForces()
{
    m_pImpl->forces.setZero();
    m_pImpl->moment = 0.0f;
}

void MCObject::integrate(MCFloat step)
{
    m_pImpl->integrate(step);
    translate(location() + velocity());
}

MCBBox<MCFloat> MCObject::bbox() const
{
    if (m_pImpl->pShape) {
        return m_pImpl->pShape->bbox();
    } else {
        return MCBBox<MCFloat>(getX(), getY(), getX() + 1, getY() + 1);
    }
}

void MCObject::stepTime()
{
    m_pImpl->time++;
}

void MCObject::beforeIntegration()
{
}

void MCObject::afterIntegration()
{
}

MCUint MCObject::time() const
{
    return m_pImpl->time;
}

void MCObject::resetTime()
{
    m_pImpl->time = 0;
}

void MCObject::setLayer(MCUint newLayer, bool updateWorldLayers)
{
    if (updateWorldLayers) {
        MCWorld::instance().removeFromLayerMap(*this);
        m_pImpl->layer = newLayer;
        MCWorld::instance().addToLayerMap(*this);
    } else {
        m_pImpl->layer = newLayer;
    }
}

MCUint MCObject::layer() const
{
    return m_pImpl->layer;
}

void MCObject::setIndex(int newIndex)
{
    m_pImpl->index = newIndex;
}

int MCObject::index() const
{
    return m_pImpl->index;
}

void MCObject::cacheIndexRange(MCUint i0, MCUint i1, MCUint j0, MCUint j1)
{
    m_pImpl->i0 = i0;
    m_pImpl->i1 = i1;
    m_pImpl->j0 = j0;
    m_pImpl->j1 = j1;
}

void MCObject::restoreIndexRange(MCUint * i0, MCUint * i1, MCUint * j0, MCUint * j1)
{
    *i0 = m_pImpl->i0;
    *i1 = m_pImpl->i1;
    *j0 = m_pImpl->j0;
    *j1 = m_pImpl->j1;
}

void MCObject::setRemoving(bool flag)
{
    m_pImpl->setFlag(RemovingMask, flag);
}

bool MCObject::removing() const
{
    return m_pImpl->flags & RemovingMask;
}

void MCObject::addContact(MCContact & contact)
{
    m_pImpl->contacts[&contact.object()].push_back(&contact);
}

const MCObject::ContactHash & MCObject::contacts() const
{
    return m_pImpl->contacts;
}

void MCObject::deleteContacts()
{
    auto i(m_pImpl->contacts.begin());
    for (; i != m_pImpl->contacts.end(); i++) {
        for (MCUint j = 0; j < i->second.size(); j++) {
            i->second[j]->free();
        }
    }
    m_pImpl->contacts.clear();
}

void MCObject::deleteContacts(MCObject & object)
{
    auto i(m_pImpl->contacts.find(&object));
    if (i != m_pImpl->contacts.end()) {
        for (MCUint j = 0; j < i->second.size(); j++) {
            i->second[j]->free();
        }
        i->second.clear();
    }
}

void MCObject::setInitialLocation(const MCVector3dF & location)
{
    m_pImpl->initialLocation = location;
}

const MCVector3dF & MCObject::initialLocation() const
{
    return m_pImpl->initialLocation;
}

void MCObject::setXYFriction(MCFloat friction)
{
    m_pImpl->xyFriction = friction;
}

MCFloat MCObject::xyFriction() const
{
    return m_pImpl->xyFriction;
}

MCObject::~MCObject()
{
    deleteContacts();

    delete m_pImpl->pShape;
    delete m_pImpl;
}
