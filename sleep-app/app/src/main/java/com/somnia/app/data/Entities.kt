package com.somnia.app.data

import androidx.room.Entity
import androidx.room.ForeignKey
import androidx.room.Index
import androidx.room.PrimaryKey

@Entity(tableName = "sessions")
data class SleepSession(
    @PrimaryKey(autoGenerate = true) val id: Long = 0,
    val startTime: Long,
    val endTime: Long? = null,
    /** Heure estimée d'endormissement / de réveil (analyse des mouvements et sons). */
    val sleepOnset: Long? = null,
    val wakeTime: Long? = null,
    val micSensitivity: Int = 6,
    val avgNoiseFloorDb: Float? = null,
    val score: Int? = null,
    val deepMinutes: Int = 0,
    val lightMinutes: Int = 0,
    val awakeMinutes: Int = 0,
    val snoringMinutes: Int = 0,
    val awakenings: Int = 0,
    /** Étiquettes du journal du soir séparées par des virgules (caféine, sport...). */
    val tags: String = "",
    /** Humeur au réveil 1..5 (0 = non renseignée). */
    val morningMood: Int = 0,
    val notes: String = "",
    val classifierName: String = "",
    /** Activité médiane de la nuit (sert à calibrer les nuits suivantes). */
    val movementMedian: Float? = null,
    /** Nombre de nuits précédentes utilisées pour calibrer l'analyse. */
    val calibrationNights: Int = 0,
)

@Entity(
    tableName = "events",
    foreignKeys = [ForeignKey(
        entity = SleepSession::class, parentColumns = ["id"], childColumns = ["sessionId"],
        onDelete = ForeignKey.CASCADE,
    )],
    indices = [Index("sessionId")],
)
data class SoundEventEntity(
    @PrimaryKey(autoGenerate = true) val id: Long = 0,
    val sessionId: Long,
    val timestamp: Long,
    val durationMs: Long,
    val type: String,
    val confidence: Float,
    val peakDb: Float,
    val noiseFloorDb: Float,
    /** Scores détaillés « TYPE=0.42;TYPE=0.10 ». */
    val scores: String,
    val audioPath: String?,
    /** Version où la voix est isolée du bruit ambiant. */
    val enhancedPath: String?,
    val favorite: Boolean = false,
    /** Paroles transcrites (hors ligne). */
    val transcript: String? = null,
    /** Type corrigé à la main par l'utilisateur. */
    val userCorrected: Boolean = false,
)

/** Statistiques minute par minute : mouvement (accéléromètre) et son. */
@Entity(
    tableName = "minutes",
    primaryKeys = ["sessionId", "minute"],
    foreignKeys = [ForeignKey(
        entity = SleepSession::class, parentColumns = ["id"], childColumns = ["sessionId"],
        onDelete = ForeignKey.CASCADE,
    )],
)
data class MinuteStat(
    val sessionId: Long,
    val minute: Int,
    val movement: Float,
    val avgDb: Float,
    val maxDb: Float,
    val events: Int,
    /** 0 = profond, 1 = léger, 2 = éveillé (rempli à la fin de la nuit). */
    val stage: Int = 1,
)
