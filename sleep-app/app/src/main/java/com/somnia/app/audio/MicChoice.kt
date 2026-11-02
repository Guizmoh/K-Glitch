package com.somnia.app.audio

import android.annotation.SuppressLint
import android.content.Context
import android.media.AudioDeviceInfo
import android.media.AudioFormat
import android.media.AudioManager
import android.media.AudioRecord
import android.media.MediaRecorder
import android.os.Build

/**
 * Choix du micro utilisé la nuit.
 *  - "auto"   : micro principal, sans traitement (par défaut)
 *  - "bottom" : micro du bas, celui des appels
 *  - "back"   : micro du haut / arrière, celui de la caméra
 *  - "all"    : tous les micros intégrés à la fois (stéréo mélangée)
 *  - "dev:ID" : un micro externe précis (casque filaire, USB, Bluetooth…)
 */
object MicChoice {
    data class Option(val key: String, val label: String, val detail: String)

    val BUILT_IN = listOf(
        Option("auto", "Automatique", "Micro principal du téléphone (conseillé)"),
        Option("bottom", "Micro des appels", "En bas du téléphone, près de la bouche"),
        Option("back", "Micro de la caméra", "En haut / au dos, à côté de l'appareil photo"),
        Option("all", "Tous les micros à la fois", "Combine les micros du téléphone (meilleure écoute de la pièce)"),
    )

    private val EXTERNAL_TYPES = buildSet {
        add(AudioDeviceInfo.TYPE_WIRED_HEADSET)
        add(AudioDeviceInfo.TYPE_USB_DEVICE)
        add(AudioDeviceInfo.TYPE_BLUETOOTH_SCO)
        if (Build.VERSION.SDK_INT >= 26) add(AudioDeviceInfo.TYPE_USB_HEADSET)
        if (Build.VERSION.SDK_INT >= 31) add(AudioDeviceInfo.TYPE_BLE_HEADSET)
    }

    /** Micros externes branchés en ce moment. */
    fun externalOptions(context: Context): List<Option> {
        val am = context.getSystemService(AudioManager::class.java)
        return am.getDevices(AudioManager.GET_DEVICES_INPUTS)
            .filter { it.type in EXTERNAL_TYPES }
            .map { d ->
                val kind = when (d.type) {
                    AudioDeviceInfo.TYPE_WIRED_HEADSET -> "Casque / micro filaire"
                    AudioDeviceInfo.TYPE_BLUETOOTH_SCO -> "Micro Bluetooth"
                    else -> if (Build.VERSION.SDK_INT >= 31 && d.type == AudioDeviceInfo.TYPE_BLE_HEADSET) "Micro Bluetooth" else "Micro USB"
                }
                Option("dev:${d.id}", kind, d.productName?.toString()?.takeIf { it.isNotBlank() } ?: "Branché sur le téléphone")
            }
    }

    fun label(context: Context, key: String): String =
        (BUILT_IN + externalOptions(context)).firstOrNull { it.key == key }?.label ?: "Automatique"

    /** Un enregistreur prêt à démarrer + le nombre de canaux à mélanger. */
    class Recorder(val record: AudioRecord, val channels: Int)

    @SuppressLint("MissingPermission")
    fun open(context: Context, key: String): Recorder {
        val sr = AudioMath.SAMPLE_RATE
        val am = context.getSystemService(AudioManager::class.java)
        val inputs = am.getDevices(AudioManager.GET_DEVICES_INPUTS)
        fun builtIn(address: String) = inputs.firstOrNull {
            it.type == AudioDeviceInfo.TYPE_BUILTIN_MIC && it.address.equals(address, ignoreCase = true)
        }
        fun make(source: Int, stereo: Boolean): AudioRecord? = runCatching {
            val ch = if (stereo) AudioFormat.CHANNEL_IN_STEREO else AudioFormat.CHANNEL_IN_MONO
            val min = AudioRecord.getMinBufferSize(sr, ch, AudioFormat.ENCODING_PCM_16BIT)
            AudioRecord(source, sr, ch, AudioFormat.ENCODING_PCM_16BIT, maxOf(min, AudioMath.FRAME * 8 * (if (stereo) 2 else 1)))
                .takeIf { it.state == AudioRecord.STATE_INITIALIZED }
        }.getOrNull()
        val unprocessed = if (Build.VERSION.SDK_INT >= 24) MediaRecorder.AudioSource.UNPROCESSED else MediaRecorder.AudioSource.MIC

        val result: Recorder? = when {
            key == "bottom" -> make(MediaRecorder.AudioSource.VOICE_RECOGNITION, false)?.also { r -> builtIn("bottom")?.let { r.preferredDevice = it } }?.let { Recorder(it, 1) }
            key == "back" -> (make(MediaRecorder.AudioSource.CAMCORDER, false))?.also { r -> builtIn("back")?.let { r.preferredDevice = it } }?.let { Recorder(it, 1) }
            key == "all" -> (make(unprocessed, true) ?: make(MediaRecorder.AudioSource.MIC, true))?.let { Recorder(it, 2) }
            key.startsWith("dev:") -> {
                val id = key.removePrefix("dev:").toIntOrNull()
                val dev = inputs.firstOrNull { it.id == id }
                make(MediaRecorder.AudioSource.VOICE_RECOGNITION, false)?.also { r -> dev?.let { r.preferredDevice = it } }?.let { Recorder(it, 1) }
            }
            else -> null
        }
        return result
            ?: make(MediaRecorder.AudioSource.VOICE_RECOGNITION, false)?.let { Recorder(it, 1) }
            ?: Recorder(make(MediaRecorder.AudioSource.MIC, false)!!, 1)
    }
}
