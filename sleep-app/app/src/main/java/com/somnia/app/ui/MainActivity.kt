package com.somnia.app.ui

import android.graphics.Color as AColor
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.SystemBarStyle
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.widthIn
import androidx.compose.material.icons.rounded.Bedtime
import androidx.compose.material.icons.rounded.CalendarMonth
import androidx.compose.material.icons.rounded.Insights
import androidx.compose.material.icons.rounded.Settings
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.NavigationBarItemDefaults
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.getValue
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import com.somnia.app.ui.screens.OnboardingScreen
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.viewmodel.compose.viewModel
import androidx.navigation.NavGraph.Companion.findStartDestination
import androidx.navigation.NavType
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.currentBackStackEntryAsState
import androidx.navigation.compose.rememberNavController
import androidx.navigation.navArgument
import com.somnia.app.ui.components.NightBackground
import com.somnia.app.ui.screens.HistoryScreen
import com.somnia.app.ui.screens.SessionScreen
import com.somnia.app.ui.screens.SettingsScreen
import com.somnia.app.ui.screens.StatsScreen
import com.somnia.app.ui.screens.TonightScreen
import com.somnia.app.ui.theme.PoliceBlueDark
import com.somnia.app.ui.theme.SomniaTheme
import com.somnia.app.ui.theme.TextSoft

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        handleWidgetIntent(intent)
        enableEdgeToEdge(
            statusBarStyle = SystemBarStyle.dark(AColor.TRANSPARENT),
            navigationBarStyle = SystemBarStyle.dark(AColor.TRANSPARENT),
        )
        setContent {
            SomniaTheme {
                val nav = rememberNavController()
                val vm: SomniaViewModel = viewModel()
                val entry by nav.currentBackStackEntryAsState()
                val route = entry?.destination?.route
                val tabs = listOf(
                    Triple("tonight", "Ce soir", androidx.compose.material.icons.Icons.Rounded.Bedtime),
                    Triple("history", "Mes nuits", androidx.compose.material.icons.Icons.Rounded.CalendarMonth),
                    Triple("stats", "Tendances", androidx.compose.material.icons.Icons.Rounded.Insights),
                    Triple("settings", "Réglages", androidx.compose.material.icons.Icons.Rounded.Settings),
                )
                val prefs = remember { getSharedPreferences("app", MODE_PRIVATE) }
                var showTuto by remember { mutableStateOf(!prefs.getBoolean("tuto_done", false)) }
                val settingsNow by vm.settings.collectAsState()
                // Icônes de la barre d'état : claires sur thème sombre, sombres sur thème clair
                val darkTheme = com.somnia.app.ui.theme.Themes.chosen.dark
                androidx.compose.runtime.LaunchedEffect(darkTheme) {
                    enableEdgeToEdge(
                        statusBarStyle = if (darkTheme) SystemBarStyle.dark(AColor.TRANSPARENT) else SystemBarStyle.light(AColor.TRANSPARENT, AColor.TRANSPARENT),
                        navigationBarStyle = if (darkTheme) SystemBarStyle.dark(AColor.TRANSPARENT) else SystemBarStyle.light(AColor.TRANSPARENT, AColor.TRANSPARENT),
                    )
                }
                androidx.compose.runtime.CompositionLocalProvider(
                    LocalAgent provides settingsNow.agent,
                    LocalAgentToggle provides { Agent.next(this@MainActivity) },
                ) {
                var showSplash by remember { mutableStateOf(savedInstanceState == null) }
                androidx.compose.animation.AnimatedVisibility(
                    showSplash, enter = androidx.compose.animation.fadeIn(), exit = androidx.compose.animation.fadeOut(),
                ) { com.somnia.app.ui.screens.SplashPoster(onDone = { showSplash = false }) }
                if (!showSplash) NightBackground {
                    if (showTuto) {
                        OnboardingScreen { prefs.edit().putBoolean("tuto_done", true).apply(); showTuto = false }
                        return@NightBackground
                    }
                    androidx.compose.foundation.layout.Box(
                        androidx.compose.ui.Modifier.fillMaxSize(),
                        contentAlignment = androidx.compose.ui.Alignment.TopCenter,
                    ) {
                    Scaffold(
                        modifier = androidx.compose.ui.Modifier.widthIn(max = 600.dp),
                        containerColor = Color.Transparent,
                        bottomBar = {
                            NavigationBar(containerColor = com.somnia.app.ui.theme.Themes.cur.nav, tonalElevation = 0.dp) {
                                tabs.forEach { (r, label, ico) ->
                                    val selected = route == r || (r == "history" && route?.startsWith("session") == true)
                                    NavigationBarItem(
                                        selected = selected,
                                        onClick = {
                                            nav.navigate(r) {
                                                popUpTo(nav.graph.findStartDestination().id) { saveState = true }
                                                launchSingleTop = true
                                                restoreState = true
                                            }
                                        },
                                        icon = { androidx.compose.material3.Icon(ico, null) },
                                        label = { Text(label) },
                                        colors = NavigationBarItemDefaults.colors(
                                            indicatorColor = com.somnia.app.ui.theme.Badge.copy(alpha = 0.16f),
                                            selectedIconColor = com.somnia.app.ui.theme.Badge,
                                            selectedTextColor = com.somnia.app.ui.theme.Badge,
                                            unselectedIconColor = TextSoft,
                                            unselectedTextColor = TextSoft,
                                        ),
                                    )
                                }
                            }
                        },
                    ) { pad ->
                        NavHost(
                            nav, startDestination = "tonight", modifier = Modifier.padding(pad),
                            enterTransition = { fadeIn() }, exitTransition = { fadeOut() },
                        ) {
                            composable("tonight") { TonightScreen(vm) { nav.navigate("session/$it") } }
                            composable("history") { HistoryScreen(vm) { nav.navigate("session/$it") } }
                            composable("stats") { StatsScreen(vm) }
                            composable("settings") { SettingsScreen(vm) { showTuto = true } }
                            composable("session/{id}", listOf(navArgument("id") { type = NavType.LongType })) {
                                SessionScreen(vm, it.arguments?.getLong("id") ?: 0L) { nav.popBackStack() }
                            }
                        }
                    }
                    }
                }
                            }
}
        }
    }

    override fun onNewIntent(intent: android.content.Intent) {
        super.onNewIntent(intent)
        handleWidgetIntent(intent)
    }

    /** Bouton « Commencer la nuit » du widget : on démarre si le micro est autorisé. */
    private fun handleWidgetIntent(intent: android.content.Intent?) {
        if (intent?.action != com.somnia.app.widget.DodoWidget.ACTION_START_NIGHT) return
        intent.action = null
        if (com.somnia.app.service.TrackingState.state.value != null) return
        val ok = androidx.core.content.ContextCompat.checkSelfPermission(this, android.Manifest.permission.RECORD_AUDIO) ==
            android.content.pm.PackageManager.PERMISSION_GRANTED
        if (ok) com.somnia.app.service.SleepTrackingService.start(this)
        else android.widget.Toast.makeText(this, "Autorisez le micro avec le bouton « Commencer la nuit ».", android.widget.Toast.LENGTH_LONG).show()
    }

    override fun onStop() {
        super.onStop()
        AudioPlayer.stop()
    }
}

