package com.somnia.app.audio

import java.io.File
import java.io.RandomAccessFile
import java.nio.ByteBuffer
import java.nio.ByteOrder

/** Lecture / écriture de fichiers WAV PCM 16 bits mono. */
object WavFile {
    fun write(file: File, pcm: ShortArray, sampleRate: Int = AudioMath.SAMPLE_RATE) {
        file.parentFile?.mkdirs()
        val dataLen = pcm.size * 2
        val bb = ByteBuffer.allocate(44 + dataLen).order(ByteOrder.LITTLE_ENDIAN)
        bb.put("RIFF".toByteArray()); bb.putInt(36 + dataLen); bb.put("WAVE".toByteArray())
        bb.put("fmt ".toByteArray()); bb.putInt(16); bb.putShort(1); bb.putShort(1)
        bb.putInt(sampleRate); bb.putInt(sampleRate * 2); bb.putShort(2); bb.putShort(16)
        bb.put("data".toByteArray()); bb.putInt(dataLen)
        for (s in pcm) bb.putShort(s)
        file.writeBytes(bb.array())
    }

    /** Fréquence d'échantillonnage écrite dans l'en-tête (16 kHz par défaut). */
    fun rate(file: File): Int = runCatching {
        RandomAccessFile(file, "r").use { raf ->
            raf.seek(24); val b = ByteArray(4); raf.readFully(b)
            ByteBuffer.wrap(b).order(ByteOrder.LITTLE_ENDIAN).int
        }
    }.getOrDefault(AudioMath.SAMPLE_RATE)

    /** Lit le fichier et le ramène à 16 kHz (analyse, transcription). */
    fun read16k(file: File): ShortArray = Resample.to16k(read(file), rate(file))

    fun read(file: File): ShortArray {
        RandomAccessFile(file, "r").use { raf ->
            val bytes = ByteArray((raf.length() - 44).coerceAtLeast(0).toInt())
            raf.seek(44)
            raf.readFully(bytes)
            val bb = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN)
            return ShortArray(bytes.size / 2) { bb.getShort() }
        }
    }
}
