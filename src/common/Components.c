#include "Components.h"

#define COMPONENT_NAME_ENTRY(Type) #Type,
static const char* ComponentTypeNames[] = {FOR_EACH(COMPONENT_NAME_ENTRY, COMPONENT_TYPE_LIST)};
_Static_assert(ARRAY_COUNT(ComponentTypeNames) == ComponentType_Count, "");

const char* ComponentTypeName(ComponentType Type)
{
	ASSERT(VALID_INDEX(Type, ComponentType_Count));
	return ComponentTypeNames[Type];
}

bool ComponentTypeTryParse(const char *TypeString, int32 TypeStringLength, ComponentType *OutType)
{
	ASSERT(OutType != NULL);

	bool Success = false;
	for (int32 ComponentTypeIndex = 0; ComponentTypeIndex < ComponentType_Count; ComponentTypeIndex++)
	{
		if (SDL_strncasecmp(ComponentTypeNames[ComponentTypeIndex], TypeString, TypeStringLength) == 0) {
			*OutType = (ComponentType)ComponentTypeIndex;
			Success = true;
		}
	}

	return Success;
}

bool ColliderIntersectsCollider(
	const ColliderComponent* A,
	Tform2 TransformA,
	const ColliderComponent* B,
	Tform2 TransformB)
{
	switch (A->Type) {
		case ColliderType_Circle:
			switch (B->Type) {
				case ColliderType_Circle: return CircleIntersectsCircle(&A->Circle, TransformA, &B->Circle, TransformB);
				case ColliderType_Polygon:
					return CircleIntersectsPolygon(&A->Circle, TransformA, &B->Polygon, TransformB);
				default: unreachable(); break;
			}
			break;

		case ColliderType_Polygon:
			switch (B->Type) {
				case ColliderType_Circle:
					return PolygonIntersectsCircle(&A->Polygon, TransformA, &B->Circle, TransformB);
				case ColliderType_Polygon:
					return PolygonIntersectsPolygon(&A->Polygon, TransformA, &B->Polygon, TransformB);
				default: unreachable(); break;
			}
			break;

		default: unreachable(); break;
	}

	return false;
}

Tform2 T2Component(const TransformComponent* Transform)
{
	ASSERT(Transform != NULL);
	return T2(Transform->Position, R2(Transform->Rotation));
}