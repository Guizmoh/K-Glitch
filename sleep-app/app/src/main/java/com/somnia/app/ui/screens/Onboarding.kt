package com.somnia.app.ui.screens

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.systemBarsPadding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.pager.HorizontalPager
import androidx.compose.foundation.pager.rememberPagerState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.runtime.withFrameNanos
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.scale
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.somnia.app.audio.SoundType
import com.somnia.app.ui.components.GradientButton
import com.somnia.app.ui.components.Pill
import com.somnia.app.ui.components.PoliceLogoFrame
import com.somnia.app.ui.theme.Badge
import com.somnia.app.ui.theme.Good
import com.somnia.app.ui.theme.PoliceBlue
import com.somnia.app.ui.theme.TextSoft
import com.somnia.app.ui.theme.color
import kotlinx.coroutines.launch
import kotlin.math.PI
import kotlin.math.abs
import kotlin.math.sin

/** Une page du tuto : titre, texte et illustration animée (fonction du temps t en secondes). */
class TutoPage(val title: String, val text: String, val art: @Composable (t: Float) -> Unit)

val TUTO_PAGES = listOf(
    TutoPage(
        "Bonsoir ma Moman",
        "Je suis l'agent du Dodo. Pendant que tu dors, je veille sur tes nuits… et je te raconte tout le matin.",
    ) { t -> HelloArt(t) },
    TutoPage(
        "J'écoute la nuit",
        "Je mesure le bruit de fond de la chambre. Dès qu'un son en sort (une parole, un cri, un ronflement), je l'enregistre. Le reste du temps, je n'enregistre rien.",
    ) { t -> WaveArt(t) },
    TutoPage(
        "Je reconnais les sons",
        "Une petite intelligence artificielle range chaque son : paroles, cris, ronflements, rires, pleurs, toux. Tout reste dans le téléphone, rien ne part sur Internet.",
    ) { t -> SoundsArt(t) },
    TutoPage(
        "Je sépare la voix du bruit",
        "Pour chaque parole, je retire le souffle du ventilateur ou de la rue. Avec « Voix isolée », on entend bien ce qui a été dit.",
    ) { t -> CleanVoiceArt(t) },
    TutoPage(
        "Comment ça marche ?",
        "",
    ) { t -> StepsArt(t) },
    TutoPage(
        "Bonjour !",
        "Je peux te réveiller quand ton sommeil est léger. Ensuite je te montre ton rapport : une note sur 100, les phases de la nuit et les sons à réécouter.",
    ) { t -> WakeArt(t) },
)

/** Horloge en secondes pour les animations. */
@Composable
fun rememberSeconds(): Float {
    var t by remember { mutableFloatStateOf(0f) }
    LaunchedEffect(Unit) {
        val start = withFrameNanos { it }
        while (true) withFrameNanos { t = (it - start) / 1e9f }
    }
    return t
}

@Composable
fun OnboardingScreen(onDone: () -> Unit) {
    val pager = rememberPagerState { TUTO_PAGES.size }
    val scope = rememberCoroutineScope()
    val t = rememberSeconds()
    Column(Modifier.fillMaxSize().systemBarsPadding()) {
        Row(Modifier.fillMaxWidth().padding(horizontal = 8.dp), horizontalArrangement = Arrangement.End) {
            if (pager.currentPage < TUTO_PAGES.lastIndex) TextButton(onClick = onDone) { Text("Passer", color = TextSoft) }
            else Spacer(Modifier.height(48.dp))
        }
        HorizontalPager(pager, Modifier.weight(1f)) { i -> TutoPageContent(TUTO_PAGES[i], if (i == pager.currentPage) t else 0f) }
        TutoFooter(pager.currentPage) {
            if (pager.currentPage == TUTO_PAGES.lastIndex) onDone()
            else scope.launch { pager.animateScrollToPage(pager.currentPage + 1) }
        }
    }
}

/** Version fixe d'une page (pour les captures et l'aperçu). */
@Composable
fun OnboardingFrame(page: Int, t: Float) {
    Column(Modifier.fillMaxSize()) {
        Spacer(Modifier.height(48.dp))
        Box(Modifier.weight(1f)) { TutoPageContent(TUTO_PAGES[page], t) }
        TutoFooter(page) {}
    }
}

@Composable
private fun TutoPageContent(p: TutoPage, t: Float) {
    Column(
        Modifier.fillMaxSize().padding(horizontal = 28.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center,
    ) {
        Box(Modifier.fillMaxWidth().height(300.dp), contentAlignment = Alignment.Center) { p.art(t) }
        Spacer(Modifier.height(24.dp))
        Text(p.title, style = MaterialTheme.typography.headlineMedium, textAlign = TextAlign.Center)
        if (p.text.isNotEmpty()) {
            Spacer(Modifier.height(12.dp))
            Text(p.text, style = MaterialTheme.typography.bodyLarge, color = TextSoft, textAlign = TextAlign.Center)
        }
    }
}

@Composable
private fun TutoFooter(page: Int, onNext: () -> Unit) {
    Column(Modifier.padding(24.dp), horizontalAlignment = Alignment.CenterHorizontally) {
        Row(horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            TUTO_PAGES.indices.forEach { i ->
                Box(
                    Modifier.height(8.dp).width(if (i == page) 24.dp else 8.dp).clip(CircleShape)
                        .background(if (i == page) PoliceBlue else Color(0x33FFFFFF)),
                )
            }
        }
        Spacer(Modifier.height(20.dp))
        GradientButton(if (page == TUTO_PAGES.lastIndex) "C'est parti !" else "Suivant", onClick = onNext)
    }
}

// ---------- Illustrations animées ----------

private fun blink(t: Float, period: Float = 3.2f): Float {
    val p = t % period
    return if (p > period - 0.18f) abs(p - (period - 0.09f)) / 0.09f else 1f
}

private fun smooth(x: Float) = x.coerceIn(0f, 1f).let { it * it * (3 - 2 * it) }

@Composable
private fun HelloArt(t: Float) {
    val bob = sin(t * 2f) * 6f
    Box(contentAlignment = Alignment.Center) {
        Canvas(Modifier.size(280.dp)) {
            repeat(9) { i ->
                val a = i * 40f * PI.toFloat() / 180f + t * 0.3f
                val r = size.minDimension / 2.1f
                val tw = 0.4f + 0.6f * abs(sin(t * 1.5f + i))
                drawCircle(Badge.copy(alpha = 0.7f * tw), 3.dp.toPx() * tw, Offset(center.x + r * kotlin.math.cos(a), center.y + r * sin(a)))
            }
        }
        Box(Modifier.padding(top = (12 + bob).coerceAtLeast(0f).dp)) { PoliceLogoFrame(200.dp, 1f) }
        Text("", fontSize = 44.sp, modifier = Modifier.align(Alignment.BottomEnd).padding(end = 36.dp, bottom = 30.dp)
            .scale(1f + 0.08f * sin(t * 6f)))
    }
}

@Composable
private fun WaveArt(t: Float) {
    val cycle = t % 4f
    val event = smooth((cycle - 1.2f) / 0.4f) * (1f - smooth((cycle - 2.8f) / 0.4f))
    Column(horizontalAlignment = Alignment.CenterHorizontally) {
        Canvas(Modifier.fillMaxWidth().height(200.dp)) {
            val n = 28
            val w = size.width / n
            val threshold = size.height * 0.55f
            drawLine(Badge, Offset(0f, threshold), Offset(size.width, threshold), 2.dp.toPx())
            for (i in 0 until n) {
                val noise = 0.12f + 0.08f * abs(sin(i * 12.9898f + t * 5f))
                val bump = event * (0.75f * kotlin.math.exp(-((i - n / 2f) * (i - n / 2f)) / 18f)) * (0.8f + 0.2f * sin(t * 14f + i))
                val h = (noise + bump) * size.height
                val loud = size.height - h < threshold
                drawRoundRect(
                    if (loud) Color(0xFFFF8A80) else PoliceBlue,
                    Offset(i * w + w * 0.2f, size.height - h), Size(w * 0.6f, h),
                    androidx.compose.ui.geometry.CornerRadius(w * 0.3f),
                )
            }
        }
        Spacer(Modifier.height(10.dp))
        Box(Modifier.alpha(if (event > 0.3f) 1f else 0.25f)) { Pill("Enregistrement en cours", Color(0xFFE08A7E)) }
        Text("— seuil au-dessus du bruit de fond", color = Badge, fontSize = 14.sp, fontWeight = FontWeight.SemiBold, modifier = Modifier.padding(top = 6.dp))
    }
}

@Composable
private fun SoundsArt(t: Float) {
    val types = listOf(SoundType.SPEECH, SoundType.NIGHT_TERROR, SoundType.SNORING, SoundType.LAUGHTER, SoundType.CRYING, SoundType.COUGH)
    val cycle = t % 6f
    Column(horizontalAlignment = Alignment.CenterHorizontally, verticalArrangement = Arrangement.spacedBy(8.dp)) {
        types.chunked(2).forEachIndexed { row, pair ->
            Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                pair.forEachIndexed { col, ty ->
                    val k = row * 2 + col
                    val a = smooth((cycle - k * 0.55f) / 0.35f)
                    Box(Modifier.alpha(a).scale(0.7f + 0.3f * a)) {
                        Text(
                            ty.label,
                            Modifier.clip(RoundedCornerShape(50)).background(ty.color().copy(alpha = 0.2f)).padding(horizontal = 16.dp, vertical = 12.dp),
                            color = ty.color(), fontWeight = FontWeight.SemiBold, fontSize = 15.sp, maxLines = 1,
                        )
                    }
                }
            }
        }
        Spacer(Modifier.height(10.dp))
        Text("100 % dans le téléphone", color = Good, fontWeight = FontWeight.SemiBold)
    }
}

@Composable
private fun CleanVoiceArt(t: Float) {
    val scan = (t % 3.5f) / 3f
    Column(horizontalAlignment = Alignment.CenterHorizontally) {
        Text("Avant", color = TextSoft, fontSize = 15.sp, fontWeight = FontWeight.SemiBold)
        WaveLine(noise = 1f, scan = 1f, t = t)
        Spacer(Modifier.height(18.dp))
        Text("Après : voix isolée", color = Good, fontSize = 15.sp, fontWeight = FontWeight.SemiBold)
        WaveLine(noise = 0f, scan = scan.coerceAtMost(1f), t = t)
    }
}

@Composable
private fun WaveLine(noise: Float, scan: Float, t: Float) {
    Canvas(Modifier.fillMaxWidth().height(100.dp)) {
        val mid = size.height / 2
        val n = 160
        var prev: Offset? = null
        for (i in 0..n) {
            val x = i / n.toFloat()
            val env = sin(PI.toFloat() * x).let { it * it }
            val voice = env * sin(x * 38f + t * 3f) * 0.75f
            val hiss = sin(i * 91.7f + t * 40f) * 0.32f
            val clean = x <= scan && noise == 0f
            val v = if (clean) voice else voice + hiss
            val p = Offset(x * size.width, mid - v * mid * 0.9f)
            prev?.let { drawLine(if (clean) Good else Color(0xFF9E9EB8), it, p, 2.dp.toPx(), StrokeCap.Round) }
            prev = p
        }
        if (noise == 0f && scan < 1f) drawLine(com.somnia.app.ui.theme.Themes.cur.text, Offset(scan * size.width, 0f), Offset(scan * size.width, size.height), 1.5.dp.toPx())
    }
}

@Composable
private fun StepsArt(t: Float) {
    val steps = listOf(
        "" to "Pose le téléphone près de ta tête, branché.",
        "" to "Appuie sur « Commencer » et dors bien.",
        "" to "Le matin, appuie sur « Je me réveille ».",
    )
    val cycle = t % 6f
    Column(horizontalAlignment = Alignment.CenterHorizontally) {
        PoliceLogoFrame(110.dp, 0f, zzzPhase = (t % 2.6f) / 2.6f, sleepFrame = ((t / 0.45f).toInt() % 6))
        Spacer(Modifier.height(16.dp))
        steps.forEachIndexed { i, (e, s) ->
            val a = smooth((cycle - i * 0.8f) / 0.4f)
            Row(
                Modifier.fillMaxWidth().padding(vertical = 5.dp).alpha(0.15f + 0.85f * a)
                    .clip(RoundedCornerShape(18.dp)).background(Color(0x1FFFFFFF)).padding(12.dp),
                verticalAlignment = Alignment.CenterVertically,
            ) {
                Box(Modifier.size(34.dp).clip(CircleShape).background(PoliceBlue), contentAlignment = Alignment.Center) {
                    Text("${i + 1}", fontWeight = FontWeight.Bold, color = com.somnia.app.ui.theme.Themes.cur.onAccent)
                }
                Text(" $e ", fontSize = 20.sp)
                Text(s, style = MaterialTheme.typography.bodyMedium)
            }
        }
    }
}

@Composable
private fun WakeArt(t: Float) {
    val cycle = t % 6f
    val open = smooth((cycle - 1.2f) / 0.6f) * blink(t)
    val score = smooth((cycle - 2f) / 1.5f)
    Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(18.dp)) {
        PoliceLogoFrame(150.dp, open, zzzPhase = if (open < 0.3f) (t % 2.6f) / 2.6f else null, sleepFrame = ((t / 0.45f).toInt() % 6))
        Box(contentAlignment = Alignment.Center) {
            Canvas(Modifier.size(110.dp)) {
                val st = 11.dp.toPx()
                val tl = Offset(st / 2, st / 2)
                val sz = Size(size.width - st, size.height - st)
                drawArc(com.somnia.app.ui.theme.Themes.cur.text.copy(alpha = 0.1f), 135f, 270f, false, tl, sz, style = Stroke(st, cap = StrokeCap.Round))
                if (score > 0.01f) drawArc(Good, 135f, 270f * 0.82f * score, false, tl, sz, style = Stroke(st, cap = StrokeCap.Round))
            }
            Text("${(82 * score).toInt()}", fontSize = 32.sp, fontWeight = FontWeight.Bold, color = Good)
        }
    }
}
