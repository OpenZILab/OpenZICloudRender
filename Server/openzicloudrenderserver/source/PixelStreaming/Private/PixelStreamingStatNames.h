// 
// Copyright by Cengzi Technology Co., Ltd
// Created by OpenZILab
// DateTime: 2022/08/26 10:40
// 

#pragma once

#include <string>

namespace OpenZI::CloudRender
{
    using FPixelStreamingStatName = std::string;

    const FPixelStreamingStatName JitterBufferDelay		    = FPixelStreamingStatName("jitterBufferDelay");
	const FPixelStreamingStatName FramesSent				= FPixelStreamingStatName("framesSent");
	const FPixelStreamingStatName FramesPerSecond			= FPixelStreamingStatName("framesPerSecond");
	const FPixelStreamingStatName FramesReceived			= FPixelStreamingStatName("framesReceived");
	const FPixelStreamingStatName FramesDecoded			    = FPixelStreamingStatName("framesDecoded");
	const FPixelStreamingStatName FramesDropped			    = FPixelStreamingStatName("framesDropped");
	const FPixelStreamingStatName FramesCorrupted			= FPixelStreamingStatName("framesCorrupted");
	const FPixelStreamingStatName PartialFramesLost		    = FPixelStreamingStatName("partialFramesLost");
	const FPixelStreamingStatName FullFramesLost		    = FPixelStreamingStatName("fullFramesLost");
	const FPixelStreamingStatName HugeFramesSent			= FPixelStreamingStatName("hugeFramesSent");
	const FPixelStreamingStatName JitterBufferTargetDelay   = FPixelStreamingStatName("jitterBufferTargetDelay");
	const FPixelStreamingStatName InterruptionCount		    = FPixelStreamingStatName("interruptionCount");
	const FPixelStreamingStatName TotalInterruptionDuration = FPixelStreamingStatName("totalInterruptionDuration");
	const FPixelStreamingStatName FreezeCount				= FPixelStreamingStatName("freezeCount");
	const FPixelStreamingStatName PauseCount				= FPixelStreamingStatName("pauseCount");
	const FPixelStreamingStatName TotalFreezesDuration	    = FPixelStreamingStatName("totalFreezesDuration");
	const FPixelStreamingStatName TotalPausesDuration		= FPixelStreamingStatName("totalPausesDuration");
	const FPixelStreamingStatName FirCount				    = FPixelStreamingStatName("firCount");
	const FPixelStreamingStatName PliCount				    = FPixelStreamingStatName("pliCount");
	const FPixelStreamingStatName NackCount				    = FPixelStreamingStatName("nackCount");
	const FPixelStreamingStatName SliCount				    = FPixelStreamingStatName("sliCount");
	const FPixelStreamingStatName RetransmittedBytesSent	= FPixelStreamingStatName("retransmittedBytesSent");
	const FPixelStreamingStatName TargetBitrate			    = FPixelStreamingStatName("targetBitrate");
	const FPixelStreamingStatName TotalEncodeBytesTarget	= FPixelStreamingStatName("totalEncodedBytesTarget");
	const FPixelStreamingStatName KeyFramesEncoded		    = FPixelStreamingStatName("keyFramesEncoded");
	const FPixelStreamingStatName FrameWidth				= FPixelStreamingStatName("frameWidth");
	const FPixelStreamingStatName FrameHeight				= FPixelStreamingStatName("frameHeight");
	const FPixelStreamingStatName BytesSent				    = FPixelStreamingStatName("bytesSent");
	const FPixelStreamingStatName QPSum					    = FPixelStreamingStatName("qpSum");
	const FPixelStreamingStatName TotalEncodeTime		    = FPixelStreamingStatName("totalEncodeTime");
	const FPixelStreamingStatName TotalPacketSendDelay	    = FPixelStreamingStatName("totalPacketSendDelay");
	const FPixelStreamingStatName FramesEncoded			    = FPixelStreamingStatName("framesEncoded");
	const FPixelStreamingStatName QualityController		    = FPixelStreamingStatName("qualityController");

	// Calculated stats
	const FPixelStreamingStatName FramesSentPerSecond	 = FPixelStreamingStatName("transmitFps");
	const FPixelStreamingStatName Bitrate				 = FPixelStreamingStatName("bitrate");
	const FPixelStreamingStatName MeanQPPerSecond		 = FPixelStreamingStatName("qp");
	const FPixelStreamingStatName MeanEncodeTime		 = FPixelStreamingStatName("encodeTime");
	const FPixelStreamingStatName EncodedFramesPerSecond = FPixelStreamingStatName("encodeFps");
	const FPixelStreamingStatName MeanSendDelay		     = FPixelStreamingStatName("captureToSend");
	const FPixelStreamingStatName SourceFps			     = FPixelStreamingStatName("captureFps");
    
} // namespace OpenZI::CloudRender