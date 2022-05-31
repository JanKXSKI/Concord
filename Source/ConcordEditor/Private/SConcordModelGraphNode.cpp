// Copyright Jan Klimaschewski. All Rights Reserved.

#include "SConcordModelGraphNode.h"

void SConcordModelGraphNode::SetDefaultTitleAreaWidget(TSharedRef<SOverlay> DefaultTitleAreaWidget)
{
    TSharedRef<SWidget> Gloss = DefaultTitleAreaWidget->GetChildren()->GetChildAt(0);
    Gloss->SetVisibility(EVisibility::Hidden);
    TSharedRef<SBorder> Spill = StaticCastSharedRef<SBorder>(DefaultTitleAreaWidget->GetChildren()->GetChildAt(1)->GetChildren()->GetChildAt(0));
    Spill->SetBorderBackgroundColor(FColor(0, 0, 0, 120));
    TSharedRef<SWidget> Highlight = DefaultTitleAreaWidget->GetChildren()->GetChildAt(2);
    Highlight->SetVisibility(EVisibility::Hidden);
}
