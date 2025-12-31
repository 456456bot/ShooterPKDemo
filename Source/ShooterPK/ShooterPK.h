// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

// --- 添加以下宏定义 ---

// 这里的 Key 设为 -1，表示打印新行；如果你想覆盖旧消息，可以改个固定数字
#define PRINT(Format, ...) { \
	const FString Msg = FString::Printf(TEXT(Format), ##__VA_ARGS__); \
	FString Prefix; \
	if (GEngine) { \
		UWorld* World = GEngine->GetWorldFromContextObject(this, EGetWorldErrorMode::ReturnNull); \
		if (World) { \
			if (World->GetNetMode() == NM_Client) { \
				/* 在编辑器里，GPlayInEditorID 能区分客户端 1, 2... */ \
				Prefix = FString::Printf(TEXT("[Client %d] "), GPlayInEditorID); \
			} else if (World->GetNetMode() == NM_DedicatedServer || World->GetNetMode() == NM_ListenServer) { \
			Prefix = TEXT("[Server] "); \
			} else { \
				Prefix = TEXT("[Standalone] "); \
			} \
		} \
	} \
FColor LogColor = (Prefix.Contains("Server")) ? FColor::Red : FColor::Cyan; \
GEngine->AddOnScreenDebugMessage(-1, 10.0f, LogColor, Prefix + Msg); \
}
