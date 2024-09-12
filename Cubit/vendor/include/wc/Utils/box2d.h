#pragma once

#include <glm/glm.hpp>
#include <box2d/box2d.h>

namespace wc 
{
	struct b2World
	{
	private:
		b2WorldId id = b2_nullWorldId;
	public:
		b2World() = default;
		b2World(b2WorldId handle) { id = handle; }
		b2World(const b2WorldDef& def) { Create(def); }

		operator b2WorldId& () { return id; }
		operator const b2WorldId& () const { return id; }
		//operator bool() const { return id != b2_nullBodyId; }

		/// Create a world for rigid body simulation. A world contains bodies, shapes, and constraints. You make create
		///	up to 128 worlds. Each world is completely independent and may be simulated in parallel.
		///	@return the world id.
		inline void Create(const b2WorldDef& def) { id = b2CreateWorld(&def); }

		/// Destroy a world
		inline void Destroy()
		{
			b2DestroyWorld(id);
			id = b2_nullWorldId;
		}

		/// World id validation. Provides validation for up to 64K allocations.
		inline bool IsValid() { return b2World_IsValid(id); }

		/// Simulate a world for one time step. This performs collision detection, integration, and constraint solution.
		/// @param worldId The world to simulate
		/// @param timeStep The amount of time to simulate, this should be a fixed number. Typically 1/60.
		/// @param subStepCount The number of sub-steps, increasing the sub-step count can increase accuracy. Typically 4.
		inline void Step(float timeStep, int subStepCount) { b2World_Step(id, timeStep, subStepCount); }

		/// Call this to draw shapes and other debug draw data
		inline void Draw(b2DebugDraw* draw) { b2World_Draw(id, draw); }

		/// Get the body events for the current time step. The event data is transient. Do not store a reference to this data.
		inline auto GetBodyEvents() { return b2World_GetBodyEvents(id); }

		/// Get sensor events for the current time step. The event data is transient. Do not store a reference to this data.
		inline auto GetSensorEvents() { return b2World_GetSensorEvents(id); }

		/// Get contact events for this current time step. The event data is transient. Do not store a reference to this data.
		inline auto GetContactEvents() { return b2World_GetContactEvents(id); }

		/// Overlap test for all shapes that *potentially* overlap the provided AABB
		inline void OverlapAABB(b2AABB aabb, b2QueryFilter filter, b2OverlapResultFcn* fcn, void* context) { b2World_OverlapAABB(id, aabb, filter, fcn, context); }

		/// Overlap test for for all shapes that overlap the provided circle
		inline void OverlapCircle(const b2Circle* circle, b2Transform transform, b2QueryFilter filter,
			b2OverlapResultFcn* fcn, void* context) {
			b2World_OverlapCircle(id, circle, transform, filter, fcn, context);
		}

		/// Overlap test for all shapes that overlap the provided capsule
		inline void OverlapCapsule(const b2Capsule* capsule, b2Transform transform, b2QueryFilter filter,
			b2OverlapResultFcn* fcn, void* context)	{
			b2World_OverlapCapsule(id, capsule, transform, filter, fcn, context);
		}

		/// Overlap test for all shapes that overlap the provided polygon
		inline void OverlapPolygon(const b2Polygon* polygon, b2Transform transform, b2QueryFilter filter,
			b2OverlapResultFcn* fcn, void* context) {
			b2World_OverlapPolygon(id, polygon, transform, filter, fcn, context);
		}

		/// Cast a ray into the world to collect shapes in the path of the ray.
		/// Your callback function controls whether you get the closest point, any point, or n-points.
		/// The ray-cast ignores shapes that contain the starting point.
		///	@param worldId The world to cast the ray against
		///	@param origin The start point of the ray
		///	@param translation The translation of the ray from the start point to the end point
		///	@param filter Contains bit flags to filter unwanted shapes from the results
		/// @param fcn A user implemented callback function
		/// @param context A user context that is passed along to the callback function
		///	@note The callback function may receive shapes in any order
		inline void CastRay(glm::vec2 origin, glm::vec2 translation, b2QueryFilter filter, b2CastResultFcn* fcn,
			void* context) {
			b2World_CastRay(id, {origin.x, origin.y}, {translation.x, translation.y}, filter, fcn, context);
		}

		/// Cast a ray into the world to collect the closest hit. This is a convenience function.
		/// This is less general than CastRay() and does not allow for custom filtering.
		inline auto CastRayClosest(glm::vec2 origin, glm::vec2 translation, b2QueryFilter filter) { return b2World_CastRayClosest(id, { origin.x, origin.y }, { translation.x, translation.y }, filter); }

		/// Cast a circle through the world. Similar to a cast ray except that a circle is cast instead of a point.
		inline void CastCircle(const b2Circle* circle, b2Transform originTransform, glm::vec2 translation,
			b2QueryFilter filter, b2CastResultFcn* fcn, void* context) {
			b2World_CastCircle(id, circle, originTransform, { translation.x, translation.y }, filter, fcn, context);
		}

		/// Cast a capsule through the world. Similar to a cast ray except that a capsule is cast instead of a point.
		inline void CastCapsule(const b2Capsule* capsule, b2Transform originTransform, glm::vec2 translation,
			b2QueryFilter filter, b2CastResultFcn* fcn, void* context) {
			b2World_CastCapsule(id, capsule, originTransform, { translation.x, translation.y }, filter, fcn, context);
		}

		/// Cast a polygon through the world. Similar to a cast ray except that a polygon is cast instead of a point.
		inline void CastPolygon(const b2Polygon* polygon, b2Transform originTransform, glm::vec2 translation,
			b2QueryFilter filter, b2CastResultFcn* fcn, void* context) {
			b2World_CastPolygon(id, polygon, originTransform, { translation.x, translation.y }, filter, fcn, context);
		}

		/// Enable/disable sleep. If your application does not need sleeping, you can gain some performance
		///	by disabling sleep completely at the world level.
		///	@see b2WorldDef
		inline void EnableSleeping(bool flag) { b2World_EnableSleeping(id, flag); }

		/// Enable/disable continuous collision between dynamic and static bodies. Generally you should keep continuous
		/// collision enabled to prevent fast moving objects from going through static objects. The performance gain from
		///	disabling continuous collision is minor.
		///	@see b2WorldDef
		inline void EnableContinuous(bool flag) { b2World_EnableContinuous(id, flag); }

		/// Adjust the restitution threshold. It is recommended not to make this value very small
		///	because it will prevent bodies from sleeping. Typically in meters per second.
		///	@see b2WorldDef
		inline void SetRestitutionThreshold(float value) { b2World_SetRestitutionThreshold(id, value); }

		/// Adjust the hit event threshold. This controls the collision velocity needed to generate a b2ContactHitEvent.
		/// Typically in meters per second.
		///	@see b2WorldDef::hitEventThreshold
		inline void SetHitEventThreshold(float value) { b2World_SetHitEventThreshold(id, value); }

		/// Register the custom filter callback. This is optional.
		inline void SetCustomFilterCallback(b2CustomFilterFcn* fcn, void* context) { b2World_SetCustomFilterCallback(id, fcn, context); }

		/// Register the pre-solve callback. This is optional.
		inline void SetPreSolveCallback(b2PreSolveFcn* fcn, void* context) { b2World_SetPreSolveCallback(id, fcn, context); }

		/// Set the gravity vector for the entire world. Box2D has no concept of an up direction and this
		/// is left as a decision for the application. Typically in m/s^2.
		///	@see b2WorldDef
		inline void SetGravity(glm::vec2 gravity) { b2World_SetGravity(id, { gravity.x, gravity.y }); }

		/// Get the gravity vector
		inline auto GetGravity() { return b2World_GetGravity(id); }

		/// Apply a radial explosion
		///	@param worldId The world id
		///	@param position The center of the explosion
		///	@param radius The radius of the explosion
		///	@param impulse The impulse of the explosion, typically in kg * m / s or N * s.
		inline void Explode(glm::vec2 position, float radius, float impulse) { b2World_Explode(id, { position.x, position.y }, radius, impulse); }

		/// Adjust contact tuning parameters
		///	@param worldId The world id
		/// @param hertz The contact stiffness (cycles per second)
		/// @param dampingRatio The contact bounciness with 1 being critical damping (non-dimensional)
		/// @param pushVelocity The maximum contact constraint push out velocity (meters per second)
		///	@note Advanced feature
		inline void SetContactTuning(float hertz, float dampingRatio, float pushVelocity) { b2World_SetContactTuning(id, hertz, dampingRatio, pushVelocity); }

		/// Enable/disable constraint warm starting. Advanced feature for testing. Disabling
		///	sleeping greatly reduces stability and provides no performance gain.
		inline void EnableWarmStarting(bool flag) { b2World_EnableWarmStarting(id, flag); }

		/// Get the current world performance profile
		inline auto GetProfile() { return b2World_GetProfile(id); }

		/// Get world counters and sizes
		inline auto GetCounters() { return b2World_GetCounters(id); }

		/// Dump memory stats to box2d_memory.txt
		inline void DumpMemoryStats() { b2World_DumpMemoryStats(id); }
	};

	struct b2Body
	{
	private:
		b2BodyId id = b2_nullBodyId;
	public:
		b2Body() = default;
		b2Body(b2BodyId handle) { id = handle; }
		b2Body(b2World world, const b2BodyDef& def) { Create(world, def); }

		operator b2BodyId& () { return id; }
		operator const b2BodyId& () const { return id; }
		//operator bool() const { return id != b2_nullBodyId; }

		inline void Create(b2World world, const b2BodyDef& def) { id = b2CreateBody(world, &def); }

		inline void Destroy()
		{
			b2DestroyBody(id);
			id = b2_nullBodyId;
		}

		inline bool IsValid() { return b2Body_IsValid(id); }

		inline auto GetType() { return b2Body_GetType(id); }

		/// Change the body type. This is an expensive operation. This automatically updates the mass
		///	properties regardless of the automatic mass setting.
		inline void SetType(b2BodyType type) { b2Body_SetType(id, type); }

		/// Set the user data for a body
		inline void SetUserData(void* userData) { b2Body_SetUserData(id, userData); }

		/// Get the user data stored in a body
		inline void* GetUserData() { return b2Body_GetUserData(id); }

		/// Get the world position of a body. This is the location of the body origin.
		inline glm::vec2 GetPosition() { auto pos = b2Body_GetPosition(id); return { pos.x,pos.y }; }

		/// Get the world rotation of a body as a cosine/sine pair (complex number)
		inline auto GetRotation() { return b2Body_GetRotation(id); }

		inline auto GetAngle() { return b2Rot_GetAngle(GetRotation()); }

		/// Get the world transform of a body.
		inline auto GetTransform() { return b2Body_GetTransform(id); }

		/// Set the world transform of a body. This acts as a teleport and is fairly expensive.
		/// @note Generally you should create a body with then intended transform.
		///	@see b2BodyDef::position and b2BodyDef::angle
		inline void SetTransform(glm::vec2 position, b2Rot rotation) { b2Body_SetTransform(id, { position.x, position.y }, rotation); }

		/// Get a local point on a body given a world point
		inline glm::vec2 GetLocalPoint(glm::vec2 worldPoint) { auto pos = b2Body_GetLocalPoint(id, { worldPoint.x, worldPoint.y }); return { pos.x,pos.y }; }

		/// Get a world point on a body given a local point
		inline glm::vec2 GetWorldPoint(glm::vec2 localPoint) { auto pos = b2Body_GetWorldPoint(id, { localPoint.x, localPoint.y }); return { pos.x,pos.y }; }

		/// Get a local vector on a body given a world vector
		inline glm::vec2 GetLocalVector(glm::vec2 worldVector) { auto pos = b2Body_GetLocalVector(id, { worldVector.x, worldVector.y }); return { pos.x,pos.y }; }

		/// Get a world vector on a body given a local vector
		inline glm::vec2 GetWorldVector(glm::vec2 localVector) { auto pos = b2Body_GetWorldVector(id, { localVector.x, localVector.y }); return { pos.x,pos.y }; }

		/// Get the linear velocity of a body's center of mass. Typically in meters per second.
		inline glm::vec2 GetLinearVelocity() { auto pos = b2Body_GetLinearVelocity(id); return { pos.x,pos.y }; }

		/// Get the angular velocity of a body in radians per second
		inline auto GetAngularVelocity() { return b2Body_GetAngularVelocity(id); }

		/// Set the linear velocity of a body. Typically in meters per second.
		inline void SetLinearVelocity(glm::vec2 linearVelocity) { b2Body_SetLinearVelocity(id, { linearVelocity.x, linearVelocity.y }); }

		/// Set the angular velocity of a body in radians per second
		inline void SetAngularVelocity(float angularVelocity) { b2Body_SetAngularVelocity(id, angularVelocity); }

		/// Apply a force at a world point. If the force is not applied at the center of mass,
		/// it will generate a torque and affect the angular velocity. This optionally wakes up the body.
		///	The force is ignored if the body is not awake.
		///	@param bodyId The body id
		/// @param force The world force vector, typically in newtons (N)
		/// @param point The world position of the point of application
		/// @param wake Option to wake up the body
		inline void ApplyForce(glm::vec2 force, glm::vec2 point, bool wake) { b2Body_ApplyForce(id, { force.x, force.y }, { point.x, point.y }, wake); }

		/// Apply a force to the center of mass. This optionally wakes up the body.
		///	The force is ignored if the body is not awake.
		///	@param bodyId The body id
		/// @param force the world force vector, usually in newtons (N).
		/// @param wake also wake up the body
		inline void ApplyForceToCenter(glm::vec2 force, bool wake) { b2Body_ApplyForceToCenter(id, { force.x, force.y }, wake); }

		/// Apply a torque. This affects the angular velocity without affecting the linear velocity.
		///	This optionally wakes the body. The torque is ignored if the body is not awake.
		///	@param bodyId The body id
		/// @param torque about the z-axis (out of the screen), typically in N*m.
		/// @param wake also wake up the body
		inline void ApplyTorque(float torque, bool wake) { b2Body_ApplyTorque(id, torque, wake); }

		/// Apply an impulse at a point. This immediately modifies the velocity.
		/// It also modifies the angular velocity if the point of application
		/// is not at the center of mass. This optionally wakes the body.
		/// The impulse is ignored if the body is not awake.
		///	@param bodyId The body id
		/// @param impulse the world impulse vector, typically in N*s or kg*m/s.
		/// @param point the world position of the point of application.
		/// @param wake also wake up the body
		///	@warning This should be used for one-shot impulses. If you need a steady force,
		/// use a force instead, which will work better with the sub-stepping solver.
		inline void ApplyLinearImpulse(glm::vec2 impulse, glm::vec2 point, bool wake) { b2Body_ApplyLinearImpulse(id, { impulse.x, impulse.y }, { point.x, point.y }, wake); }

		/// Apply an impulse to the center of mass. This immediately modifies the velocity.
		/// The impulse is ignored if the body is not awake. This optionally wakes the body.
		///	@param bodyId The body id
		/// @param impulse the world impulse vector, typically in N*s or kg*m/s.
		/// @param wake also wake up the body
		///	@warning This should be used for one-shot impulses. If you need a steady force,
		/// use a force instead, which will work better with the sub-stepping solver.
		inline void ApplyLinearImpulseToCenter(glm::vec2 impulse, bool wake) { b2Body_ApplyLinearImpulseToCenter(id, { impulse.x, impulse.y }, wake); }

		/// Apply an angular impulse. The impulse is ignored if the body is not awake.
		/// This optionally wakes the body.
		///	@param bodyId The body id
		/// @param impulse the angular impulse, typically in units of kg*m*m/s
		/// @param wake also wake up the body
		///	@warning This should be used for one-shot impulses. If you need a steady force,
		/// use a force instead, which will work better with the sub-stepping solver.
		inline void ApplyAngularImpulse(float impulse, bool wake) { b2Body_ApplyAngularImpulse(id, impulse, wake); }

		/// Get the mass of the body, typically in kilograms
		inline auto GetMass() { return b2Body_GetMass(id); }

		/// Get the inertia tensor of the body, typically in kg*m^2
		inline auto GetInertiaTensor() { return b2Body_GetInertiaTensor(id); }

		/// Get the center of mass position of the body in local space
		inline glm::vec2 GetLocalCenterOfMass() { auto pos = b2Body_GetLocalCenterOfMass(id); return { pos.x,pos.y }; }

		/// Get the center of mass position of the body in world space
		inline glm::vec2 GetWorldCenterOfMass() { auto pos = b2Body_GetWorldCenterOfMass(id); return { pos.x,pos.y }; }

		/// Override the body's mass properties. Normally this is computed automatically using the
		///	shape geometry and density. This information is lost if a shape is added or removed or if the
		///	body type changes.
		inline void SetMassData(b2MassData massData) { b2Body_SetMassData(id, massData); }

		/// Get the mass data for a body
		inline auto GetMassData() { return b2Body_GetMassData(id); }

		/// This update the mass properties to the sum of the mass properties of the shapes.
		/// This normally does not need to be called unless you called SetMassData to override
		/// the mass and you later want to reset the mass.
		///	You may also use this when automatic mass computation has been disabled.
		///	You should call this regardless of body type.
		inline void ApplyMassFromShapes() { b2Body_ApplyMassFromShapes(id); }

		/// Set the automatic mass setting. Normally this is set in b2BodyDef before creation.
		///	@see b2BodyDef::automaticMass
		inline void SetAutomaticMass(bool automaticMass) { b2Body_SetAutomaticMass(id, automaticMass); }

		/// Get the automatic mass setting
		inline auto GetAutomaticMass() { return b2Body_GetAutomaticMass(id); }

		/// Adjust the linear damping. Normally this is set in b2BodyDef before creation.
		inline void SetLinearDamping(float linearDamping) { b2Body_SetLinearDamping(id, linearDamping); }

		/// Get the current linear damping.
		inline auto GetLinearDamping() { return b2Body_GetLinearDamping(id); }

		/// Adjust the angular damping. Normally this is set in b2BodyDef before creation.
		inline void SetAngularDamping(float angularDamping) { b2Body_SetAngularDamping(id, angularDamping); }

		/// Get the current angular damping.
		inline auto GetAngularDamping() { return b2Body_GetAngularDamping(id); }

		/// Adjust the gravity scale. Normally this is set in b2BodyDef before creation.
		///	@see b2BodyDef::gravityScale
		inline void SetGravityScale(float gravityScale) { b2Body_SetGravityScale(id, gravityScale); }

		/// Get the current gravity scale
		inline auto GetGravityScale() { return b2Body_GetGravityScale(id); }

		/// @return true if this body is awake
		inline bool IsAwake() { return b2Body_IsAwake(id); }

		/// Wake a body from sleep. This wakes the entire island the body is touching.
		///	@warning Putting a body to sleep will put the entire island of bodies touching this body to sleep,
		///	which can be expensive and possibly unintuitive.
		inline void SetAwake(bool awake) { return b2Body_SetAwake(id, awake); }

		/// Enable or disable sleeping for this body. If sleeping is disabled the body will wake.
		inline void EnableSleep(bool enableSleep) { b2Body_EnableSleep(id, enableSleep); }

		/// Returns true if sleeping is enabled for this body
		inline bool IsSleepEnabled() { return b2Body_IsSleepEnabled(id); }

		/// Set the sleep threshold, typically in meters per second
		inline void SetSleepThreshold(float sleepVelocity) { b2Body_SetSleepThreshold(id, sleepVelocity); }

		/// Get the sleep threshold, typically in meters per second.
		inline auto GetSleepThreshold() { return b2Body_GetSleepThreshold(id); }

		/// Returns true if this body is enabled
		inline bool IsEnabled() { return b2Body_IsEnabled(id); }

		/// Disable a body by removing it completely from the simulation. This is expensive.
		inline void Disable() { b2Body_Disable(id); }

		/// Enable a body by adding it to the simulation. This is expensive.
		inline void Enable() { b2Body_Enable(id); }

		/// Set this body to have fixed rotation. This causes the mass to be reset in all cases.
		inline void SetFixedRotation(bool flag) { b2Body_SetFixedRotation(id, flag); }

		/// Does this body have fixed rotation?
		inline bool IsFixedRotation() { return b2Body_IsFixedRotation(id); }

		/// Set this body to be a bullet. A bullet does continuous collision detection
		/// against dynamic bodies (but not other bullets).
		inline void SetBullet(bool flag) { b2Body_SetBullet(id, flag); }

		/// Is this body a bullet?
		inline bool IsBullet() { return b2Body_IsBullet(id); }

		/// Enable/disable hit events on all shapes
		///	@see b2ShapeDef::enableHitEvents
		inline void EnableHitEvents(bool enableHitEvents) { b2Body_EnableHitEvents(id, enableHitEvents); }

		/// Get the number of shapes on this body
		inline auto GetShapeCount() { return b2Body_GetShapeCount(id); }

		/// Get the shape ids for all shapes on this body, up to the provided capacity.
		///	@returns the number of shape ids stored in the user array
		inline auto GetShapes(b2ShapeId* shapeArray, int capacity) { return b2Body_GetShapes(id, shapeArray, capacity); }

		/// Get the number of joints on this body
		inline auto GetJointCount() { return b2Body_GetJointCount(id); }

		/// Get the joint ids for all joints on this body, up to the provided capacity
		///	@returns the number of joint ids stored in the user array
		inline auto GetJoints(b2JointId* jointArray, int capacity) { return b2Body_GetJoints(id, jointArray, capacity); }

		/// Get the maximum capacity required for retrieving all the touching contacts on a body
		inline auto GetContactCapacity() { return b2Body_GetContactCapacity(id); }

		/// Get the touching contact data for a body
		inline auto GetContactData(b2ContactData* contactData, int capacity) { return b2Body_GetContactData(id, contactData, capacity); }

		/// Get the current world AABB that contains all the attached shapes. Note that this may not encompass the body origin.
		///	If there are no shapes attached then the returned AABB is empty and centered on the body origin.
		inline auto ComputeAABB() { return b2Body_ComputeAABB(id); }
	};

	struct b2Shape
	{
	private:
		b2ShapeId id = b2_nullShapeId;
	public:
		b2Shape() = default;
		b2Shape(b2ShapeId handle) { id = handle; }
		b2Shape(b2Body body, const b2ShapeDef& def, const b2Circle& circle) { CreateCircleShape(body, def, circle); }
		b2Shape(b2Body body, const b2ShapeDef& def, const b2Segment& segment) { CreateSegmentShape(body, def, segment); }
		b2Shape(b2Body body, const b2ShapeDef& def, const b2Capsule& capsule) { CreateCapsuleShape(body, def, capsule); }
		b2Shape(b2Body body, const b2ShapeDef& def, const b2Polygon& polygon) { CreatePolygonShape(body, def, polygon); }

		operator b2ShapeId& () { return id; }
		operator const b2ShapeId& () const { return id; }
		//operator bool() const { return id != b2_nullBodyId; }

		inline void Destroy()
		{
			b2DestroyShape(id);
			id = b2_nullShapeId;
		}

		/// Create a circle shape and attach it to a body. The shape definition and geometry are fully cloned.
		/// Contacts are not created until the next time step.
		///	@return the shape id for accessing the shape
		inline void CreateCircleShape(b2BodyId bodyId, const b2ShapeDef& def, const b2Circle& circle) { id = b2CreateCircleShape(bodyId, &def, &circle); }

		/// Create a line segment shape and attach it to a body. The shape definition and geometry are fully cloned.
		/// Contacts are not created until the next time step.
		///	@return the shape id for accessing the shape
		inline void CreateSegmentShape(b2BodyId bodyId, const b2ShapeDef& def, const b2Segment& segment) { id = b2CreateSegmentShape(bodyId, &def, &segment); }

		/// Create a capsule shape and attach it to a body. The shape definition and geometry are fully cloned.
		/// Contacts are not created until the next time step.
		///	@return the shape id for accessing the shape
		inline void CreateCapsuleShape(b2BodyId bodyId, const b2ShapeDef& def, const b2Capsule& capsule) { id = b2CreateCapsuleShape(bodyId, &def, &capsule); }

		/// Create a polygon shape and attach it to a body. The shape definition and geometry are fully cloned.
		/// Contacts are not created until the next time step.
		///	@return the shape id for accessing the shape
		inline void CreatePolygonShape(b2BodyId bodyId, const b2ShapeDef& def, const b2Polygon& polygon) { id = b2CreatePolygonShape(bodyId, &def, &polygon); }

		/// Shape identifier validation. Provides validation for up to 64K allocations.
		inline bool IsValid() { return b2Shape_IsValid(id); }

		/// Get the type of a shape
		inline auto GetType() { return b2Shape_GetType(id); }

		/// Get the id of the body that a shape is attached to
		inline auto GetBody() { return b2Shape_GetBody(id); }

		/// Returns true If the shape is a sensor
		inline bool IsSensor() { return b2Shape_IsSensor(id); }

		/// Set the user data for a shape
		inline void SetUserData(void* userData) { b2Shape_SetUserData(id, userData); }

		/// Get the user data for a shape. This is useful when you get a shape id
		///	from an event or query.
		inline void* GetUserData() { return b2Shape_GetUserData(id); }

		/// Set the mass density of a shape, typically in kg/m^2.
		///	This will not update the mass properties on the parent body.
		///	@see b2ShapeDef::density, b2Body_ApplyMassFromShapes
		inline void SetDensity(float density) { b2Shape_SetDensity(id, density); }

		/// Get the density of a shape, typically in kg/m^2
		inline float GetDensity() { return b2Shape_GetDensity(id); }

		/// Set the friction on a shape
		///	@see b2ShapeDef::friction
		inline void SetFriction(float friction) { b2Shape_SetFriction(id, friction); }

		/// Get the friction of a shape
		inline float GetFriction() { return b2Shape_GetFriction(id); }

		/// Set the shape restitution (bounciness)
		///	@see b2ShapeDef::restitution
		inline void SetRestitution(float restitution) { b2Shape_SetRestitution(id, restitution); }

		/// Get the shape restitution
		inline float GetRestitution() { return b2Shape_GetRestitution(id); }

		/// Get the shape filter
		inline auto GetFilter() { return b2Shape_GetFilter(id); }

		/// Set the current filter. This is almost as expensive as recreating the shape.
		///	@see b2ShapeDef::filter
		inline void SetFilter(b2Filter filter) { b2Shape_SetFilter(id, filter); }

		/// Enable sensor events for this shape. Only applies to kinematic and dynamic bodies. Ignored for sensors.
		///	@see b2ShapeDef::isSensor
		inline void EnableSensorEvents(bool flag) { b2Shape_EnableSensorEvents(id, flag); }

		/// Returns true if sensor events are enabled
		inline bool AreSensorEventsEnabled() { return b2Shape_AreSensorEventsEnabled(id); }

		/// Enable contact events for this shape. Only applies to kinematic and dynamic bodies. Ignored for sensors.
		///	@see b2ShapeDef::enableContactEvents
		inline void EnableContactEvents(bool flag) { b2Shape_EnableContactEvents(id, flag); }

		/// Returns true if contact events are enabled
		inline bool AreContactEventsEnabled() { return b2Shape_AreContactEventsEnabled(id); }

		/// Enable pre-solve contact events for this shape. Only applies to dynamic bodies. These are expensive
		///	and must be carefully handled due to multithreading. Ignored for sensors.
		///	@see b2PreSolveFcn
		inline void EnablePreSolveEvents(bool flag) { b2Shape_EnablePreSolveEvents(id, flag); }

		/// Returns true if pre-solve events are enabled
		inline bool ArePreSolveEventsEnabled() { return b2Shape_ArePreSolveEventsEnabled(id); }

		/// Enable contact hit events for this shape. Ignored for sensors.
		///	@see b2WorldDef.hitEventThreshold
		inline void EnableHitEvents(bool flag) { b2Shape_EnableHitEvents(id, flag); }

		/// Returns true if hit events are enabled
		inline bool AreHitEventsEnabled() { return b2Shape_AreHitEventsEnabled(id); }

		/// Test a point for overlap with a shape
		inline bool TestPoint(glm::vec2 point) { return b2Shape_TestPoint(id, { point.x, point.y }); }

		/// Ray cast a shape directly
		inline auto RayCast(glm::vec2 origin, glm::vec2 translation) { return b2Shape_RayCast(id, { origin.x, origin.y }, { translation.x, translation.y }); }

		/// Get a copy of the shape's circle. Asserts the type is correct.
		inline auto GetCircle() { return b2Shape_GetCircle(id); }

		/// Get a copy of the shape's line segment. Asserts the type is correct.
		inline auto GetSegment() { return b2Shape_GetSegment(id); }

		/// Get a copy of the shape's smooth line segment. These come from chain shapes.
		/// Asserts the type is correct.
		inline auto GetSmoothSegment() { return b2Shape_GetSmoothSegment(id); }

		/// Get a copy of the shape's capsule. Asserts the type is correct.
		inline auto GetCapsule() { return b2Shape_GetCapsule(id); }

		/// Get a copy of the shape's convex polygon. Asserts the type is correct.
		inline auto GetPolygon() { return b2Shape_GetPolygon(id); }

		/// Allows you to change a shape to be a circle or update the current circle.
		/// This does not modify the mass properties.
		///	@see b2Body_ApplyMassFromShapes
		inline void SetCircle(const b2Circle* circle) { b2Shape_SetCircle(id, circle); }

		/// Allows you to change a shape to be a capsule or update the current capsule.
		/// This does not modify the mass properties.
		///	@see b2Body_ApplyMassFromShapes
		inline void SetCapsule(const b2Capsule* capsule) { b2Shape_SetCapsule(id, capsule); }

		/// Allows you to change a shape to be a segment or update the current segment.
		inline void SetSegment(const b2Segment* segment) { b2Shape_SetSegment(id, segment); }

		/// Allows you to change a shape to be a polygon or update the current polygon.
		/// This does not modify the mass properties.
		///	@see b2Body_ApplyMassFromShapes
		inline void SetPolygon(const b2Polygon* polygon) { b2Shape_SetPolygon(id, polygon); }

		/// Get the parent chain id if the shape type is b2_smoothSegmentShape, otherwise
		/// returns b2_nullChainId.
		inline auto GetParentChain() { return b2Shape_GetParentChain(id); }

		/// Get the maximum capacity required for retrieving all the touching contacts on a shape
		inline auto GetContactCapacity() { return b2Shape_GetContactCapacity(id); }

		/// Get the touching contact data for a shape. The provided shapeId will be either shapeIdA or shapeIdB on the contact data.
		inline auto GetContactData(b2ContactData* contactData, int capacity) { return b2Shape_GetContactData(id, contactData, capacity); }

		/// Get the current world AABB
		inline auto GetAABB() { return b2Shape_GetAABB(id); }

		/// Get the closest point on a shape to a target point. Target and result are in world space.
		inline auto GetClosestPoint(glm::vec2 target) { return b2Shape_GetClosestPoint(id, { target.x, target.y }); }
	};
}