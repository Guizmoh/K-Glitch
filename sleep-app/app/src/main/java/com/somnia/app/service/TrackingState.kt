package com.somnia.app.service

import com.somnia.app.audio.SoundType
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow

/** État en direct du suivi, observé par l'interface. */
data class LiveState(
    val sessionId: Long,
    val startTime: Long,
    val levelDb: Float = -90f,
    val noiseFloorDb: Float = -60f,
    val thresholdDb: Float = -50f,
    val capturing: Boolean = false,
    val eventCount: Int = 0,
    val counts: Map<SoundType, Int> = emptyMap(),
    val lastEvent: SoundType? = null,
    val lastEventTime: Long? = null,
    val detectionStartsAt: Long = 0,
    val classifierName: String = "",
    val alarmTarget: Long? = null,
    val movementLastMin: Float = 0f,
)

object TrackingState {
    val live = MutableStateFlow<LiveState?>(null)
    val state: StateFlow<LiveState?> get() = live
}
