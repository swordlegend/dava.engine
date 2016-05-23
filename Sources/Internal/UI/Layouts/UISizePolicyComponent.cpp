#include "UISizePolicyComponent.h"

#include "UI/UIControl.h"
#include "Math/Vector.h"

namespace DAVA
{
UISizePolicyComponent::UISizePolicyComponent()
{
    const float32 DEFAULT_VALUE = 100.0f;
    const float32 MIN_LIMIT = 0.0f;
    const float32 MAX_LIMIT = 99999.0f;

    for (int32 i = 0; i < Vector2::AXIS_COUNT; i++)
    {
        policy[i].policy = IGNORE_SIZE;
        policy[i].value = DEFAULT_VALUE;
        policy[i].min = MIN_LIMIT;
        policy[i].max = MAX_LIMIT;
    }
}

UISizePolicyComponent::UISizePolicyComponent(const UISizePolicyComponent& src)
{
    for (int32 i = 0; i < Vector2::AXIS_COUNT; i++)
    {
        policy[i].policy = src.policy[i].policy;
        policy[i].value = src.policy[i].value;
        policy[i].min = src.policy[i].min;
        policy[i].max = src.policy[i].max;
    }
}

UISizePolicyComponent::~UISizePolicyComponent()
{
}

UISizePolicyComponent* UISizePolicyComponent::Clone() const
{
    return new UISizePolicyComponent(*this);
}

UISizePolicyComponent::eSizePolicy UISizePolicyComponent::GetHorizontalPolicy() const
{
    return policy[Vector2::AXIS_X].policy;
}

void UISizePolicyComponent::SetHorizontalPolicy(eSizePolicy newPolicy)
{
    policy[Vector2::AXIS_X].policy = newPolicy;
    SetLayoutDirty();
}

float32 UISizePolicyComponent::GetHorizontalValue() const
{
    return policy[Vector2::AXIS_X].value;
}

void UISizePolicyComponent::SetHorizontalValue(float32 value)
{
    policy[Vector2::AXIS_X].value = value;
    SetLayoutDirty();
}

float32 UISizePolicyComponent::GetHorizontalMinValue() const
{
    return policy[Vector2::AXIS_X].min;
}

void UISizePolicyComponent::SetHorizontalMinValue(float32 value)
{
    policy[Vector2::AXIS_X].min = value;
    SetLayoutDirty();
}

float32 UISizePolicyComponent::GetHorizontalMaxValue() const
{
    return policy[Vector2::AXIS_X].max;
}

void UISizePolicyComponent::SetHorizontalMaxValue(float32 value)
{
    policy[Vector2::AXIS_X].max = value;
    SetLayoutDirty();
}

UISizePolicyComponent::eSizePolicy UISizePolicyComponent::GetVerticalPolicy() const
{
    return policy[Vector2::AXIS_Y].policy;
}

void UISizePolicyComponent::SetVerticalPolicy(eSizePolicy newPolicy)
{
    policy[Vector2::AXIS_Y].policy = newPolicy;
    SetLayoutDirty();
}

float32 UISizePolicyComponent::GetVerticalValue() const
{
    return policy[Vector2::AXIS_Y].value;
}

void UISizePolicyComponent::SetVerticalValue(float32 value)
{
    policy[Vector2::AXIS_Y].value = value;
    SetLayoutDirty();
}

float32 UISizePolicyComponent::GetVerticalMinValue() const
{
    return policy[Vector2::AXIS_Y].min;
}

void UISizePolicyComponent::SetVerticalMinValue(float32 value)
{
    policy[Vector2::AXIS_Y].min = value;
    SetLayoutDirty();
}

float32 UISizePolicyComponent::GetVerticalMaxValue() const
{
    return policy[Vector2::AXIS_Y].max;
}

void UISizePolicyComponent::SetVerticalMaxValue(float32 value)
{
    policy[Vector2::AXIS_Y].max = value;
    SetLayoutDirty();
}

UISizePolicyComponent::eSizePolicy UISizePolicyComponent::GetPolicyByAxis(int32 axis) const
{
    DVASSERT(0 <= axis && axis < Vector2::AXIS_COUNT);
    return policy[axis].policy;
}

float32 UISizePolicyComponent::GetValueByAxis(int32 axis) const
{
    DVASSERT(0 <= axis && axis < Vector2::AXIS_COUNT);
    return policy[axis].value;
}

float32 UISizePolicyComponent::GetMinValueByAxis(int32 axis) const
{
    DVASSERT(0 <= axis && axis < Vector2::AXIS_COUNT);
    return policy[axis].min;
}

float32 UISizePolicyComponent::GetMaxValueByAxis(int32 axis) const
{
    DVASSERT(0 <= axis && axis < Vector2::AXIS_COUNT);
    return policy[axis].max;
}

bool UISizePolicyComponent::IsDependsOnChildren(int32 axis) const
{
    DVASSERT(0 <= axis && axis < Vector2::AXIS_COUNT);
    eSizePolicy p = policy[axis].policy;
    return p == PERCENT_OF_CHILDREN_SUM || p == PERCENT_OF_MAX_CHILD || p == PERCENT_OF_FIRST_CHILD || p == PERCENT_OF_LAST_CHILD;
}

int32 UISizePolicyComponent::GetHorizontalPolicyAsInt() const
{
    return GetHorizontalPolicy();
}

void UISizePolicyComponent::SetHorizontalPolicyFromInt(int32 policy)
{
    SetHorizontalPolicy(static_cast<eSizePolicy>(policy));
}

int32 UISizePolicyComponent::GetVerticalPolicyAsInt() const
{
    return GetVerticalPolicy();
}

void UISizePolicyComponent::SetVerticalPolicyFromInt(int32 policy)
{
    SetVerticalPolicy(static_cast<eSizePolicy>(policy));
}

void UISizePolicyComponent::SetLayoutDirty()
{
    if (GetControl() != nullptr)
    {
        GetControl()->SetLayoutDirty();
    }
}
}
