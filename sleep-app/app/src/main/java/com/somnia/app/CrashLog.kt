package com.somnia.app

import android.content.Context
import android.os.Build
import java.io.File
import java.io.PrintWriter
import java.io.StringWriter
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/**
 * Garde la trace du dernier plantage (fichier local) pour pouvoir l'envoyer
 * depuis Réglages > Aide. Rien n'est envoyé automatiquement.
 */
object CrashLog {
    private fun file(ctx: Context) = File(ctx.filesDir, "dernier-plantage.txt")

    fun install(ctx: Context) {
        val app = ctx.applicationContext
        val previous = Thread.getDefaultUncaughtExceptionHandler()
        Thread.setDefaultUncaughtExceptionHandler { thread, e ->
            runCatching { file(app).writeText(format(app, thread.name, e)) }
            previous?.uncaughtException(thread, e)
        }
    }

    fun format(ctx: Context, thread: String, e: Throwable): String {
        val sw = StringWriter(); e.printStackTrace(PrintWriter(sw))
        val version = runCatching { ctx.packageManager.getPackageInfo(ctx.packageName, 0).versionName }.getOrNull()
        return buildString {
            appendLine("Police du Dodo $version — ${SimpleDateFormat("dd/MM/yyyy HH:mm", Locale.FRANCE).format(Date())}")
            appendLine("Téléphone : ${Build.MANUFACTURER} ${Build.MODEL}, Android ${Build.VERSION.RELEASE} (API ${Build.VERSION.SDK_INT})")
            appendLine("Fil : $thread")
            appendLine()
            append(sw.toString().take(12_000))
        }
    }

    fun read(ctx: Context): String? = file(ctx).takeIf { it.exists() }?.readText()
    fun clear(ctx: Context) { file(ctx).delete() }
}
