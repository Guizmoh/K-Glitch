package com.somnia.app.report

import android.content.Context
import android.content.Intent
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Typeface
import android.graphics.pdf.PdfDocument
import androidx.core.content.FileProvider
import com.somnia.app.audio.SoundType
import com.somnia.app.data.SomniaDatabase
import com.somnia.app.data.SoundEventEntity
import com.somnia.app.ui.Fmt
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import java.io.File
import kotlin.math.roundToInt

/**
 * Rapport PDF pour une consultation : 2 à 4 semaines de nuits (durée, phases, ronflement,
 * pauses respiratoires, paroles nocturnes) + extraits sonores joints (numérotés dans le PDF).
 */
object PdfReport {
    private const val W = 595; private const val H = 842; private const val M = 40

    suspend fun share(context: Context, weeks: Int) {
        val files = withContext(Dispatchers.IO) { build(context, weeks) }
        val uris = ArrayList(files.map { FileProvider.getUriForFile(context, "${context.packageName}.files", it) })
        val send = Intent(Intent.ACTION_SEND_MULTIPLE).setType("*/*")
            .putParcelableArrayListExtra(Intent.EXTRA_STREAM, uris)
            .putExtra(Intent.EXTRA_SUBJECT, "Rapport de sommeil – Police du Dodo")
            .putExtra(Intent.EXTRA_TEXT, "Rapport de sommeil sur $weeks semaines (PDF) et extraits sonores numérotés.")
            .addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
        send.clipData = android.content.ClipData.newRawUri("", uris[0]).apply { uris.drop(1).forEach { addItem(android.content.ClipData.Item(it)) } }
        context.startActivity(Intent.createChooser(send, "Envoyer le rapport").addFlags(Intent.FLAG_ACTIVITY_NEW_TASK))
    }

    /** Fabrique le PDF et les extraits ; renvoie [pdf, extrait1, extrait2, …]. */
    suspend fun build(context: Context, weeks: Int): List<File> {
        val dao = SomniaDatabase.get(context).dao()
        val since = System.currentTimeMillis() - weeks * 7 * 86_400_000L
        val nights = dao.finishedSessions().filter { it.startTime >= since }.sortedBy { it.startTime }
        val events = dao.allEvents().filter { e -> nights.any { it.id == e.sessionId } }
        val dir = File(context.cacheDir, "exports/rapport").apply { deleteRecursively(); mkdirs() }

        // Extraits : pauses respiratoires, cris, paroles, favoris (25 au plus, les plus marquants d'abord)
        val rank = mapOf(SoundType.APNEA.name to 0, SoundType.NIGHT_TERROR.name to 1, SoundType.SPEECH.name to 2)
        val picked = events.filter { (it.type in rank || it.favorite) && (it.audioPath != null) }
            .sortedWith(compareBy<SoundEventEntity>({ rank[it.type] ?: 3 }, { it.timestamp })).take(25).sortedBy { it.timestamp }
        val extracts = picked.mapIndexed { i, e ->
            val f = File(dir, "extrait_%02d_%s.wav".format(i + 1, SoundType.fromName(e.type).label.substringBefore(' ').lowercase()))
            File(e.enhancedPath ?: e.audioPath!!).copyTo(f, overwrite = true); e to f
        }

        val doc = PdfDocument()
        var pageNo = 0
        var page: PdfDocument.Page? = null
        var y = 0f
        val text = Paint().apply { color = Color.rgb(30, 30, 50); textSize = 10f; isAntiAlias = true }
        val bold = Paint(text).apply { typeface = Typeface.DEFAULT_BOLD }
        val title = Paint(bold).apply { textSize = 18f }
        val h2 = Paint(bold).apply { textSize = 13f; color = Color.rgb(60, 60, 140) }
        val soft = Paint(text).apply { color = Color.rgb(110, 110, 130); textSize = 8.5f }
        fun newPage() {
            page?.let { doc.finishPage(it) }
            pageNo++
            page = doc.startPage(PdfDocument.PageInfo.Builder(W, H, pageNo).create())
            y = M.toFloat()
            page!!.canvas.drawText("Police du Dodo – rapport de sommeil · page $pageNo", M.toFloat(), (H - 20).toFloat(), soft)
        }
        fun need(h: Float) { if (page == null || y + h > H - 50) newPage() }
        fun line(s: String, p: Paint = text, dy: Float = 14f) { need(dy); page!!.canvas.drawText(s, M.toFloat(), y, p); y += dy }

        newPage()
        line("Rapport de sommeil", title, 26f)
        if (nights.isNotEmpty()) line("Du ${Fmt.day(nights.first().startTime)} au ${Fmt.day(nights.last().startTime)} · ${nights.size} nuit(s) enregistrée(s)", soft, 20f)
        else line("Aucune nuit sur la période.", text)

        if (nights.isNotEmpty()) {
            val asleep = nights.map { it.deepMinutes + it.lightMinutes }
            val inBed = nights.map { it.deepMinutes + it.lightMinutes + it.awakeMinutes }.map { it.coerceAtLeast(1) }
            val deepPct = nights.sumOf { it.deepMinutes } * 100 / inBed.sum()
            val apneas = events.count { it.type == SoundType.APNEA.name }
            val speech = events.count { it.type == SoundType.SPEECH.name || it.type == SoundType.NIGHT_TERROR.name }
            val apneaPerH = if (asleep.sum() > 0) apneas / (asleep.sum() / 60f) else 0f
            line("Résumé", h2, 18f)
            line("• Sommeil moyen : ${Fmt.duration(asleep.average().roundToInt())} (min ${Fmt.duration(asleep.min())}, max ${Fmt.duration(asleep.max())})")
            line("• Sommeil profond : $deepPct % du temps au lit · réveils moyens : ${"%.1f".format(nights.map { it.awakenings }.average())} / nuit")
            line("• Ronflement moyen : ${nights.map { it.snoringMinutes }.average().roundToInt()} min / nuit")
            line("• Pauses respiratoires suspectes : $apneas (%.1f par heure de sommeil)".format(apneaPerH))
            line("• Paroles / cris nocturnes : $speech épisode(s)")
            nights.mapNotNull { it.score }.takeIf { it.isNotEmpty() }?.let { line("• Score moyen (estimation de l'appli) : ${it.average().roundToInt()} / 100") }
            y += 8f

            // Graphique : durée de sommeil par nuit
            line("Durée de sommeil par nuit", h2, 18f)
            need(120f)
            val c = page!!.canvas
            val chartTop = y; val chartH = 90f; val chartW = (W - 2 * M).toFloat()
            val maxH = maxOf(600, asleep.max()).toFloat()
            val bar = Paint().apply { color = Color.rgb(120, 120, 210) }
            val deep = Paint().apply { color = Color.rgb(70, 80, 170) }
            val slot = chartW / nights.size
            nights.forEachIndexed { i, s ->
                val h = chartH * (s.deepMinutes + s.lightMinutes) / maxH
                val hd = chartH * s.deepMinutes / maxH
                val x = M + i * slot
                c.drawRect(x + slot * 0.15f, chartTop + chartH - h, x + slot * 0.85f, chartTop + chartH, bar)
                c.drawRect(x + slot * 0.15f, chartTop + chartH - hd, x + slot * 0.85f, chartTop + chartH, deep)
            }
            val goal = Paint().apply { color = Color.rgb(200, 160, 60); strokeWidth = 1f }
            c.drawLine(M.toFloat(), chartTop + chartH - chartH * 480 / maxH, M + chartW, chartTop + chartH - chartH * 480 / maxH, goal)
            y = chartTop + chartH + 12f
            line("Barres : sommeil total (foncé = profond) · ligne : 8 h", soft, 18f)

            // Tableau des nuits
            line("Détail par nuit", h2, 18f)
            val cols = floatArrayOf(0f, 95f, 165f, 220f, 275f, 330f, 395f, 455f)
            fun row(values: List<String>, p: Paint) {
                need(14f); values.forEachIndexed { i, v -> page!!.canvas.drawText(v, M + cols[i], y, p) }; y += 14f
            }
            row(listOf("Nuit", "Coucher–lever", "Sommeil", "Profond", "Réveils", "Ronflem.", "Pauses/h", "Paroles"), bold)
            nights.forEach { s ->
                val a = s.deepMinutes + s.lightMinutes
                val inb = (a + s.awakeMinutes).coerceAtLeast(1)
                val ev = events.filter { it.sessionId == s.id }
                val p = ev.count { it.type == SoundType.APNEA.name }
                row(
                    listOf(
                        Fmt.shortDay(s.startTime), "${Fmt.time(s.startTime)}–${s.endTime?.let { Fmt.time(it) } ?: "?"}", Fmt.duration(a),
                        "${s.deepMinutes * 100 / inb} %", "${s.awakenings}", "${s.snoringMinutes} min",
                        if (a > 30) "%.1f".format(p / (a / 60f)) else "–",
                        "${ev.count { it.type == SoundType.SPEECH.name || it.type == SoundType.NIGHT_TERROR.name }}",
                    ),
                    text,
                )
            }
            y += 10f
            line("Extraits sonores joints", h2, 18f)
            if (extracts.isEmpty()) line("Aucun extrait marquant sur la période.", soft)
            extracts.forEachIndexed { i, (e, f) ->
                val t = SoundType.fromName(e.type)
                val extra = when {
                    t == SoundType.APNEA -> e.scores.substringAfter("APNEA_GAP=", "").takeIf { it.isNotEmpty() }?.let { " – silence de ${it.substringBefore(';').toFloat().roundToInt()} s" } ?: ""
                    e.transcript != null -> " – « ${e.transcript.take(70)} »"
                    else -> ""
                }
                line("${i + 1}. ${f.name} : ${t.label}, ${Fmt.day(e.timestamp)} à ${Fmt.time(e.timestamp)} (${Fmt.seconds(e.durationMs)})$extra")
            }
        }
        y += 12f
        line("Données mesurées par un téléphone (micro et accéléromètre) : ce sont des estimations,", soft, 12f)
        line("pas un examen médical. Les pauses respiratoires sont repérées au son seulement.", soft, 12f)
        page?.let { doc.finishPage(it) }
        val pdf = File(dir, "rapport_sommeil.pdf")
        pdf.outputStream().use { doc.writeTo(it) }
        doc.close()
        return listOf(pdf) + extracts.map { it.second }
    }
}
