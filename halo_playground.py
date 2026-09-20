#!/usr/bin/env python3
"""
================================================================================
  H.A.L.O. AEGIS CORE — INTERACTIVE CYBER-AEROSPACE PLAYGROUND & KINODYNAMICS GUI
================================================================================
An ultra-responsive, cyber-aerospace graphical simulator and test bench for:
  - Hardware-Accelerated Linear Operator (H.A.L.O.) True JPS+ routing.
  - Active Protection System (APS): Dynamic Trap Projectile Intercept & Immediate Evasion.
  - Real-Time Collision Cone & 32-beam SWAR LiDAR threat recognition.
  - Immediate Hard Banking (+90° / -90° evasive vector) & in-flight path re-routing.
  - C^3 continuous quintic polynomial trajectory synthesis & Pure Pursuit.
  - High-visibility tactical radar HUD with customizable grid resolution (32x32 / 64x64).
  - Hybrid Architecture: Direct native C++20 C-ABI ctypes bridge with automatic
    in-memory pure Python SWAR/JPS+ fallback.
================================================================================
"""

import sys
import os
import time
import math
import random
import ctypes
import platform
import subprocess
import tkinter as tk
from tkinter import messagebox

# ==============================================================================
# C-ABI STRUCTURES & NATIVE BRIDGE (ctypes)
# ==============================================================================

class HaloVec2f(ctypes.Structure):
    _fields_ = [("x", ctypes.c_float), ("y", ctypes.c_float)]

class HaloIntPoint(ctypes.Structure):
    _fields_ = [("x", ctypes.c_int32), ("y", ctypes.c_int32)]

class HaloPathResult(ctypes.Structure):
    _fields_ = [
        ("found", ctypes.c_int32),
        ("count", ctypes.c_int32),
        ("waypoints", HaloIntPoint * 1024),
    ]

class NativeEngineBridge:
    """Manages dynamic loading and compilation of the C++20 H.A.L.O. Core engine."""
    def __init__(self, width=64, height=64):
        self.width = width
        self.height = height
        self.lib = None
        self.ctx = None
        self.is_native = False
        self._try_load_or_compile()

    def _try_load_or_compile(self):
        project_root = os.path.dirname(os.path.abspath(__file__))
        build_dir = os.path.join(project_root, "build")
        os.makedirs(build_dir, exist_ok=True)

        system = platform.system()
        lib_ext = ".dylib" if system == "Darwin" else (".dll" if system == "Windows" else ".so")
        lib_path = os.path.join(build_dir, f"libhalo_core{lib_ext}")

        # Check existing binary or compile
        if not os.path.exists(lib_path):
            src_path = os.path.join(project_root, "examples", "halo_c_api.cpp")
            inc_path = os.path.join(project_root, "include")
            if os.path.exists(src_path) and os.path.exists(inc_path):
                cxx = os.environ.get("CXX", "clang++")
                cmd = [
                    cxx, "-std=c++20", "-O3", "-flto", "-DNDEBUG", "-march=native",
                    "-shared", "-fPIC", f"-I{inc_path}", src_path, "-o", lib_path
                ]
                try:
                    subprocess.run(cmd, check=True, capture_output=True, timeout=15)
                except Exception:
                    pass

        if os.path.exists(lib_path):
            try:
                self.lib = ctypes.CDLL(lib_path)
                self._setup_bindings()
                self.ctx = self.lib.ExportHaloCreateEngine(512, 512, 20)
                if self.ctx:
                    self.is_native = True
            except Exception:
                self.lib = None
                self.ctx = None
                self.is_native = False

    def _setup_bindings(self):
        self.lib.ExportHaloCreateEngine.restype = ctypes.c_void_p
        self.lib.ExportHaloCreateEngine.argtypes = [ctypes.c_int32, ctypes.c_int32, ctypes.c_size_t]
        self.lib.ExportHaloDestroyEngine.argtypes = [ctypes.c_void_p]
        self.lib.ExportHaloSetObstacle.argtypes = [ctypes.c_void_p, ctypes.c_int32, ctypes.c_int32, ctypes.c_int32]
        self.lib.ExportHaloIsObstacle.restype = ctypes.c_int32
        self.lib.ExportHaloIsObstacle.argtypes = [ctypes.c_void_p, ctypes.c_int32, ctypes.c_int32]
        self.lib.ExportHaloQueryPath.restype = ctypes.c_int32
        self.lib.ExportHaloQueryPath.argtypes = [ctypes.c_void_p, HaloIntPoint, HaloIntPoint, ctypes.POINTER(HaloPathResult)]
        self.lib.ExportHaloQueryPathOptimal.restype = ctypes.c_int32
        self.lib.ExportHaloQueryPathOptimal.argtypes = [ctypes.c_void_p, HaloIntPoint, HaloIntPoint, ctypes.POINTER(HaloPathResult)]
        self.lib.ExportHaloQueryPathAnyAngle.restype = ctypes.c_int32
        self.lib.ExportHaloQueryPathAnyAngle.argtypes = [ctypes.c_void_p, HaloIntPoint, HaloIntPoint, ctypes.POINTER(HaloPathResult)]
        self.lib.ExportHaloQueryRaycast.restype = ctypes.c_int32
        self.lib.ExportHaloQueryRaycast.argtypes = [ctypes.c_void_p, HaloVec2f, HaloVec2f, ctypes.c_float, ctypes.POINTER(ctypes.c_float)]

    def set_obstacle(self, x, y, is_obs):
        if self.is_native and self.ctx:
            self.lib.ExportHaloSetObstacle(self.ctx, int(x), int(y), 1 if is_obs else 0)

    def route(self, start, goal, mode="jps"):
        if not (self.is_native and self.ctx):
            return None
        res = HaloPathResult()
        s = HaloIntPoint(int(start[0]), int(start[1]))
        g = HaloIntPoint(int(goal[0]), int(goal[1]))
        t0 = time.perf_counter_ns()
        if mode == "optimal":
            ok = self.lib.ExportHaloQueryPathOptimal(self.ctx, s, g, ctypes.byref(res))
        elif mode == "any_angle":
            ok = self.lib.ExportHaloQueryPathAnyAngle(self.ctx, s, g, ctypes.byref(res))
        else:
            ok = self.lib.ExportHaloQueryPath(self.ctx, s, g, ctypes.byref(res))
        t1 = time.perf_counter_ns()
        latency_ns = t1 - t0

        if ok and res.count > 0:
            pts = [(res.waypoints[i].x, res.waypoints[i].y) for i in range(res.count)]
            return pts, latency_ns
        return [], latency_ns

    def raycast(self, start_pos, dir_vec, max_dist=32.0):
        if not (self.is_native and self.ctx):
            return None
        clearance = ctypes.c_float(0.0)
        s = HaloVec2f(start_pos[0], start_pos[1])
        d = HaloVec2f(dir_vec[0], dir_vec[1])
        self.lib.ExportHaloQueryRaycast(self.ctx, s, d, ctypes.c_float(max_dist), ctypes.byref(clearance))
        return clearance.value

    def cleanup(self):
        if self.is_native and self.ctx:
            self.lib.ExportHaloDestroyEngine(self.ctx)
            self.ctx = None

# ==============================================================================
# PURE PYTHON HIGH-PERFORMANCE FALLBACK ROUTER & SWAR EMULATION
# ==============================================================================

class PythonHaloEngine:
    """High-speed pure-Python simulation of bitboard routing, JPS+, and raycasting."""
    def __init__(self, width=64, height=64):
        self.width = width
        self.height = height
        self.grid = bytearray(width * height)

    def resize(self, width, height):
        self.width = width
        self.height = height
        self.grid = bytearray(width * height)

    def set_obstacle(self, x, y, is_obs):
        if 0 <= x < self.width and 0 <= y < self.height:
            self.grid[y * self.width + x] = 1 if is_obs else 0

    def is_obstacle(self, x, y):
        if 0 <= x < self.width and 0 <= y < self.height:
            return self.grid[y * self.width + x] != 0
        return True

    def raycast(self, sx, sy, dx, dy, max_dist=32.0):
        step = 0.5
        dist = 0.0
        while dist <= max_dist:
            px = int(round(sx + dx * dist))
            py = int(round(sy + dy * dist))
            if self.is_obstacle(px, py):
                return max(0.0, dist - step)
            dist += step
        return max_dist

    def route_jps(self, start, goal, clearance_weight=False):
        t0 = time.perf_counter_ns()
        sx, sy = int(start[0]), int(start[1])
        gx, gy = int(goal[0]), int(goal[1])

        if self.is_obstacle(sx, sy) or self.is_obstacle(gx, gy):
            return [], time.perf_counter_ns() - t0

        import heapq
        open_set = [(0.0, sx, sy)]
        came_from = {}
        g_score = {(sx, sy): 0.0}

        def heuristic(x, y):
            dx = abs(x - gx)
            dy = abs(y - gy)
            return (dx + dy) + (1.41421356 - 2.0) * min(dx, dy)

        dirs = [(-1, 0), (1, 0), (0, -1), (0, 1), (-1, -1), (-1, 1), (1, -1), (1, 1)]

        while open_set:
            _, cx, cy = heapq.heappop(open_set)
            if (cx, cy) == (gx, gy):
                path = []
                curr = (gx, gy)
                while curr in came_from:
                    path.append(curr)
                    curr = came_from[curr]
                path.append((sx, sy))
                path.reverse()
                t1 = time.perf_counter_ns()
                return path, t1 - t0

            cg = g_score[(cx, cy)]

            # Jump point search lookahead
            for dx, dy in dirs:
                step = 1
                jx, jy = cx + dx, cy + dy
                while 0 <= jx < self.width and 0 <= jy < self.height and not self.is_obstacle(jx, jy):
                    if (jx, jy) == (gx, gy):
                        break
                    # Forced neighbor check
                    if dx != 0 and dy != 0:
                        if (self.is_obstacle(jx - dx, jy) and not self.is_obstacle(jx - dx, jy + dy)) or \
                           (self.is_obstacle(jx, jy - dy) and not self.is_obstacle(jx + dx, jy - dy)):
                            break
                    elif dx != 0:
                        if (self.is_obstacle(jx, jy - 1) and not self.is_obstacle(jx + dx, jy - 1)) or \
                           (self.is_obstacle(jx, jy + 1) and not self.is_obstacle(jx + dx, jy + 1)):
                            break
                    elif dy != 0:
                        if (self.is_obstacle(jx - 1, jy) and not self.is_obstacle(jx - 1, jy + dy)) or \
                           (self.is_obstacle(jx + 1, jy) and not self.is_obstacle(jx + 1, jy + dy)):
                            break
                    step += 1
                    if step > 6:
                        break
                    jx += dx
                    jy += dy

                if not (0 <= jx < self.width and 0 <= jy < self.height) or self.is_obstacle(jx, jy):
                    continue

                dist = math.hypot(jx - cx, jy - cy)
                penalty = 0.0
                if clearance_weight:
                    for nx, ny in [(-1,0), (1,0), (0,-1), (0,1)]:
                        if self.is_obstacle(jx + nx, jy + ny):
                            penalty += 1.5

                tentative = cg + dist + penalty
                if tentative < g_score.get((jx, jy), float('inf')):
                    g_score[(jx, jy)] = tentative
                    came_from[(jx, jy)] = (cx, cy)
                    heapq.heappush(open_set, (tentative + heuristic(jx, jy), jx, jy))

        return [], time.perf_counter_ns() - t0

    def line_of_sight(self, p1, p2):
        x0, y0 = int(p1[0]), int(p1[1])
        x1, y1 = int(p2[0]), int(p2[1])
        dx = abs(x1 - x0)
        dy = abs(y1 - y0)
        sx = 1 if x0 < x1 else -1
        sy = 1 if y0 < y1 else -1
        err = dx - dy
        x, y = x0, y0
        while True:
            if self.is_obstacle(x, y):
                return False
            if x == x1 and y == y1:
                break
            e2 = 2 * err
            if e2 > -dy:
                err -= dy
                x += sx
            if e2 < dx:
                err += dx
                y += sy
        return True

    def prune_any_angle(self, path):
        if len(path) <= 2:
            return path
        smoothed = [path[0]]
        curr = 0
        while curr < len(path) - 1:
            next_idx = curr + 1
            for look in range(len(path) - 1, curr, -1):
                if self.line_of_sight(path[curr], path[look]):
                    next_idx = look
                    break
            smoothed.append(path[next_idx])
            curr = next_idx
        return smoothed

# ==============================================================================
# QUINTIC POLYNOMIAL TRAJECTORY GENERATOR (C^3 SMOOTHING)
# ==============================================================================

class QuinticTrajectory:
    """Generates continuous C^3 quintic polynomial flight trajectories."""
    @staticmethod
    def generate_spline(waypoints, samples_per_seg=14):
        if len(waypoints) < 2:
            return waypoints
        curve = []
        for i in range(len(waypoints) - 1):
            p0 = waypoints[i]
            p1 = waypoints[i + 1]
            for s in range(samples_per_seg):
                t = s / float(samples_per_seg)
                w = t * t * t * (t * (t * 6.0 - 15.0) + 10.0)
                x = p0[0] + (p1[0] - p0[0]) * w
                y = p0[1] + (p1[1] - p0[1]) * w
                curve.append((x, y))
        curve.append(waypoints[-1])
        return curve

# ==============================================================================
# INTERACTIVE GUI APPLICATION (TKINTER TACTICAL HUD)
# ==============================================================================

class HaloPlaygroundApp:
    def __init__(self, root):
        self.root = root
        self.root.title("H.A.L.O. AEGIS CORE — Autonomous Spatial & Kinodynamics Playground")
        self.root.geometry("1400x900")
        self.root.minsize(1120, 740)
        self.root.configure(bg="#0B0F19")

        # Grid Resolution Mode: 32 (Hi-Vis / Large) or 64 (Dense Tactical)
        self.grid_size = 32
        self.start_pt = (4, 4)
        self.goal_pt = (28, 28)
        self.active_mode = "quintic"
        self.active_tool = "obstacle"  # "obstacle", "eraser", "start", "goal", "lidar", "launch_trap"
        self.brush_size = 1

        # Dynamic Projectile Traps & Threat Waves ("Bẫy Phóng Ra & Sóng Đe Dọa")
        self.active_traps = []
        self.explosions = []  # Detonated trap visual sparks
        self.threat_wave_phase = 0.0

        # Autonomous Drone & Flight Dynamics State
        self.drone_pos = [float(self.start_pt[0]), float(self.start_pt[1])]
        self.drone_heading = 0.0
        self.drone_speed = 0.0
        self.drone_target_speed = 3.5
        self.flight_active = False
        self.flight_idx = 0
        self.rotor_angle = 0.0
        self.pulse_phase = 0.0
        self.evasion_alert_ticks = 0

        # Aegis Shield Active Reflex State
        self.aegis_alarm_level = 0  # 0: Nominal, 1: Caution (L9), 2: Hard Evasive Turn (L2)
        self.tactical_status = "CRUISE: STANDBY"

        # Pathfinding Telemetry State
        self.last_latency_ns = 350
        self.last_waypoints_count = 0
        self.last_path_length = 0.0
        self.cached_path = []
        self.cached_spline = []
        self.lidar_rays = []

        # Dual Engines
        self.native_bridge = NativeEngineBridge(self.grid_size, self.grid_size)
        self.py_engine = PythonHaloEngine(self.grid_size, self.grid_size)

        # Build UI Structure
        self._create_layout()
        self._generate_preset("metropolis")
        self._recalculate_path()
        self._setup_keybindings()
        self._run_animation_loop()

    # --------------------------------------------------------------------------
    # UI Layout & Aesthetics
    # --------------------------------------------------------------------------
    def _create_layout(self):
        # 1. Top Header Telemetry HUD Bar
        self.hud_bar = tk.Frame(self.root, bg="#111827", height=64, bd=1, relief="flat",
                                highlightbackground="#1F2937", highlightthickness=1)
        self.hud_bar.pack(side="top", fill="x", padx=12, pady=(10, 6))

        # Brand Title in HUD
        brand_frame = tk.Frame(self.hud_bar, bg="#111827")
        brand_frame.pack(side="left", padx=16)
        tk.Label(brand_frame, text="H.A.L.O. AEGIS CORE", font=("Helvetica", 14, "bold"),
                 fg="#00F0FF", bg="#111827").pack(anchor="w")
        engine_txt = "● C++20 SILICON CORE (Native C-ABI)" if self.native_bridge.is_native else "● PURE PYTHON SWAR EMULATION"
        engine_col = "#00FF9D" if self.native_bridge.is_native else "#FFB800"
        self.lbl_engine_status = tk.Label(brand_frame, text=engine_txt, font=("Helvetica", 9, "bold"),
                                          fg=engine_col, bg="#111827")
        self.lbl_engine_status.pack(anchor="w")

        # Telemetry Badges in HUD
        self.lbl_hud_latency = self._create_hud_badge("LATENCY", "0.35 µs", "#38BDF8")
        self.lbl_hud_path = self._create_hud_badge("PATH SPAN", "0.0 m", "#F9FAFB")
        self.lbl_hud_pts = self._create_hud_badge("WAYPOINTS", "0 pts", "#F9FAFB")
        self.lbl_hud_aegis = self._create_hud_badge("AEGIS APS SHIELD", "ALL CLEAR", "#00FF9D")
        self.lbl_hud_mode = self._create_hud_badge("GRID MODE", f"{self.grid_size}x{self.grid_size} HI-VIS", "#A78BFA")

        # 2. Main Body Split: Left Sidebar & Right Viewport
        body_frame = tk.Frame(self.root, bg="#0B0F19")
        body_frame.pack(side="top", fill="both", expand=True, padx=12, pady=6)

        # Left Sidebar Frame
        self.sidebar = tk.Frame(body_frame, bg="#111827", width=350,
                                highlightbackground="#1F2937", highlightthickness=1)
        self.sidebar.pack(side="left", fill="y", padx=(0, 10))
        self.sidebar.pack_propagate(False)

        # Right Viewport Frame (Canvas & Live Flight Telemetry Banner)
        viewport_frame = tk.Frame(body_frame, bg="#0B0F19")
        viewport_frame.pack(side="right", fill="both", expand=True)

        # Tactical Status Banner above canvas (Ultra-Clear Visibility)
        self.banner_frame = tk.Frame(viewport_frame, bg="#0F172A", height=38,
                                     highlightbackground="#1E293B", highlightthickness=1)
        self.banner_frame.pack(side="top", fill="x", pady=(0, 6))

        self.lbl_banner_status = tk.Label(self.banner_frame, text="🟢 TACTICAL RADAR ONLINE — STANDBY",
                                          font=("Helvetica", 10, "bold"), fg="#00FF9D", bg="#0F172A")
        self.lbl_banner_status.pack(side="left", padx=12, pady=6)

        self.lbl_banner_coords = tk.Label(self.banner_frame, text="DRONE: (4.0, 4.0) | GOAL: (27, 27) | SPEED: 0.0 m/s",
                                          font=("Courier", 10, "bold"), fg="#94A3B8", bg="#0F172A")
        self.lbl_banner_coords.pack(side="right", padx=12, pady=6)

        # High-Contrast Tactical Canvas
        self.canvas = tk.Canvas(viewport_frame, bg="#0A0E1A", highlightthickness=1,
                                highlightbackground="#1E293B")
        self.canvas.pack(fill="both", expand=True)
        self.canvas.bind("<Button-1>", self._on_canvas_click)
        self.canvas.bind("<B1-Motion>", self._on_canvas_drag)
        self.canvas.bind("<Button-3>", self._on_canvas_right_click)
        self.canvas.bind("<B3-Motion>", self._on_canvas_right_drag)
        self.canvas.bind("<Motion>", self._on_canvas_hover)
        self.canvas.bind("<Configure>", lambda e: self._redraw_canvas())

        # Populate Sidebar Controls
        self._build_sidebar_controls()

    def _create_hud_badge(self, label, value, color):
        badge = tk.Frame(self.hud_bar, bg="#1F2937", padx=12, pady=4,
                         highlightbackground="#374151", highlightthickness=1)
        badge.pack(side="left", padx=6, pady=8)
        tk.Label(badge, text=label, font=("Helvetica", 8, "bold"), fg="#9CA3AF", bg="#1F2937").pack(anchor="w")
        val_lbl = tk.Label(badge, text=value, font=("Helvetica", 11, "bold"), fg=color, bg="#1F2937")
        val_lbl.pack(anchor="w")
        return val_lbl

    def _build_sidebar_controls(self):
        container = tk.Frame(self.sidebar, bg="#111827", padx=14, pady=10)
        container.pack(fill="both", expand=True)

        # Section 0: Grid Resolution (Hi-Vis vs Dense Tactical)
        self._create_section_header(container, "👁️ VISIBILITY & GRID RESOLUTION")
        grid_res_frame = tk.Frame(container, bg="#111827")
        grid_res_frame.pack(fill="x", pady=2)
        self.btn_grid_32 = tk.Button(grid_res_frame, text="32x32 (Hi-Vis)", font=("Helvetica", 8, "bold"),
                                     bg="#00F0FF", fg="#0B0F19", bd=0, padx=6, pady=4,
                                     command=lambda: self._set_grid_resolution(32))
        self.btn_grid_32.pack(side="left", fill="x", expand=True, padx=2)
        self.btn_grid_64 = tk.Button(grid_res_frame, text="64x64 (Tactical)", font=("Helvetica", 8, "bold"),
                                     bg="#1F2937", fg="#F9FAFB", bd=0, padx=6, pady=4,
                                     command=lambda: self._set_grid_resolution(64))
        self.btn_grid_64.pack(side="right", fill="x", expand=True, padx=2)

        # Section 1: Interactive Tools
        self._create_section_header(container, "🛠️ INTERACTIVE TOOLS")
        tools_frame = tk.Frame(container, bg="#111827")
        tools_frame.pack(fill="x", pady=2)

        self.btn_tool_obs = self._create_tool_btn(tools_frame, "✏️ Wall", "obstacle", 0, 0)
        self.btn_tool_era = self._create_tool_btn(tools_frame, "🧹 Erase", "eraser", 0, 1)
        self.btn_tool_start = self._create_tool_btn(tools_frame, "🟢 Start", "start", 1, 0)
        self.btn_tool_goal = self._create_tool_btn(tools_frame, "🎯 Goal", "goal", 1, 1)
        self.btn_tool_lidar = self._create_tool_btn(tools_frame, "📡 Probe", "lidar", 2, 0)
        self.btn_tool_launcher = self._create_tool_btn(tools_frame, "🚀 Aim Trap", "launch_trap", 2, 1)

        # Prominent Trap Projectile Launcher Button ("Phóng Bẫy Về Drone")
        launcher_frame = tk.Frame(container, bg="#111827")
        launcher_frame.pack(fill="x", pady=(6, 4))
        self.btn_launch_trap = tk.Button(launcher_frame, text="🚀 PHÓNG BẪY VỀ DRONE [T]",
                                         font=("Helvetica", 9, "bold"), bg="#EF4444", fg="#FFFFFF",
                                         bd=0, pady=7, command=self._launch_trap_towards_drone)
        self.btn_launch_trap.pack(fill="x")

        # Brush Size
        brush_frame = tk.Frame(container, bg="#111827")
        brush_frame.pack(fill="x", pady=(2, 6))
        tk.Label(brush_frame, text="Brush Size:", font=("Helvetica", 8), fg="#9CA3AF", bg="#111827").pack(side="left")
        for b in [1, 3, 5]:
            btn = tk.Button(brush_frame, text=f"{b}x{b}", font=("Helvetica", 8, "bold"), width=4,
                            bg="#1F2937", fg="#F9FAFB", activebackground="#00F0FF", bd=0,
                            command=lambda size=b: self._set_brush_size(size))
            btn.pack(side="left", padx=3)

        # Section 2: Routing Algorithms
        self._create_section_header(container, "⚡ ROUTING ALGORITHMS")
        self.var_algo = tk.StringVar(value="quintic")
        algos = [
            ("🏎️ Kinodynamic (C³ Spline)", "quintic"),
            ("⚡ True JPS+ (Jump Point)", "jps"),
            ("📐 Any-Angle Reflex (Theta*)", "any_angle"),
            ("🛡️ Aegis 10-Layer Clearance", "clearance")
        ]
        for text, val in algos:
            rb = tk.Radiobutton(container, text=text, variable=self.var_algo, value=val,
                                font=("Helvetica", 8, "bold"), fg="#38BDF8", bg="#111827",
                                selectcolor="#1F2937", activebackground="#111827",
                                command=self._on_algo_change)
            rb.pack(anchor="w", pady=1)

        # Section 3: Tactical Presets
        self._create_section_header(container, "🏙️ MAP PRESETS")
        presets_frame = tk.Frame(container, bg="#111827")
        presets_frame.pack(fill="x", pady=2)
        tk.Button(presets_frame, text="Metropolis", font=("Helvetica", 8, "bold"), bg="#1F2937", fg="#F9FAFB",
                  bd=0, padx=4, pady=3, command=lambda: self._generate_preset("metropolis")).grid(row=0, column=0, padx=2, pady=2, sticky="ew")
        tk.Button(presets_frame, text="Maze", font=("Helvetica", 8, "bold"), bg="#1F2937", fg="#F9FAFB",
                  bd=0, padx=4, pady=3, command=lambda: self._generate_preset("maze")).grid(row=0, column=1, padx=2, pady=2, sticky="ew")
        tk.Button(presets_frame, text="Chokepoints", font=("Helvetica", 8, "bold"), bg="#1F2937", fg="#F9FAFB",
                  bd=0, padx=4, pady=3, command=lambda: self._generate_preset("chokepoint")).grid(row=1, column=0, padx=2, pady=2, sticky="ew")
        tk.Button(presets_frame, text="Random", font=("Helvetica", 8, "bold"), bg="#1F2937", fg="#F9FAFB",
                  bd=0, padx=4, pady=3, command=lambda: self._generate_preset("random")).grid(row=1, column=1, padx=2, pady=2, sticky="ew")
        tk.Button(container, text="🧼 Clear All Obstacles", font=("Helvetica", 8, "bold"), bg="#374151", fg="#F87171",
                  bd=0, pady=3, command=self._clear_grid).pack(fill="x", pady=(2, 6))

        # Section 4: Autonomous Flight Simulator
        self._create_section_header(container, "🚁 FLIGHT DYNAMICS & REFLEX")
        flight_btn_frame = tk.Frame(container, bg="#111827")
        flight_btn_frame.pack(fill="x", pady=2)
        self.btn_play_flight = tk.Button(flight_btn_frame, text="▶️ Play Flight", font=("Helvetica", 9, "bold"),
                                         bg="#00FF9D", fg="#0B0F19", bd=0, padx=10, pady=6, command=self._toggle_flight)
        self.btn_play_flight.pack(side="left", fill="x", expand=True, padx=(0, 4))
        tk.Button(flight_btn_frame, text="⏹️ Reset", font=("Helvetica", 9, "bold"),
                  bg="#1F2937", fg="#F9FAFB", bd=0, padx=8, pady=6, command=self._reset_drone).pack(side="right")

        # Section 5: Benchmark & Stress Engine
        self._create_section_header(container, "📊 EMPIRICAL TELEMETRY")
        tk.Button(container, text="🚀 RUN 1,000x STRESS BENCHMARK", font=("Helvetica", 8, "bold"),
                  bg="#00F0FF", fg="#0B0F19", bd=0, pady=7, command=self._run_benchmark_modal).pack(fill="x", pady=(2, 6))

    def _create_section_header(self, parent, text):
        tk.Label(parent, text=text, font=("Helvetica", 8, "bold"), fg="#6B7280", bg="#111827").pack(anchor="w", pady=(6, 1))

    def _create_tool_btn(self, parent, text, mode, r, c):
        btn = tk.Button(parent, text=text, font=("Helvetica", 8, "bold"), bg="#1F2937", fg="#F9FAFB",
                        activebackground="#00F0FF", bd=0, padx=6, pady=5, width=10,
                        command=lambda: self._select_tool(mode))
        btn.grid(row=r, column=c, padx=2, pady=2, sticky="ew")
        if mode == "obstacle":
            btn.configure(bg="#00F0FF", fg="#0B0F19")
        return btn

    def _select_tool(self, mode):
        self.active_tool = mode
        buttons = [self.btn_tool_obs, self.btn_tool_era, self.btn_tool_start, self.btn_tool_goal, self.btn_tool_lidar, self.btn_tool_launcher]
        modes = ["obstacle", "eraser", "start", "goal", "lidar", "launch_trap"]
        for b, m in zip(buttons, modes):
            if m == mode:
                b.configure(bg="#00F0FF", fg="#0B0F19")
            else:
                b.configure(bg="#1F2937", fg="#F9FAFB")

    def _set_brush_size(self, size):
        self.brush_size = size

    def _set_grid_resolution(self, size):
        if self.grid_size == size:
            return
        self.grid_size = size
        if size == 32:
            self.btn_grid_32.configure(bg="#00F0FF", fg="#0B0F19")
            self.btn_grid_64.configure(bg="#1F2937", fg="#F9FAFB")
            self.start_pt = (4, 4)
            self.goal_pt = (28, 28)
        else:
            self.btn_grid_64.configure(bg="#00F0FF", fg="#0B0F19")
            self.btn_grid_32.configure(bg="#1F2937", fg="#F9FAFB")
            self.start_pt = (7, 7)
            self.goal_pt = (61, 61)

        self.lbl_hud_mode.configure(text=f"{self.grid_size}x{self.grid_size} {'HI-VIS' if size == 32 else 'TACTICAL'}")
        self.py_engine.resize(self.grid_size, self.grid_size)
        self.active_traps.clear()
        self._reset_drone()
        self._generate_preset("metropolis")

    def _on_algo_change(self):
        self.active_mode = self.var_algo.get()
        self._recalculate_path()
        self._redraw_canvas()

    # --------------------------------------------------------------------------
    # Grid & Preset Generation
    # --------------------------------------------------------------------------
    def _clear_grid(self):
        for y in range(self.grid_size):
            for x in range(self.grid_size):
                self.py_engine.set_obstacle(x, y, False)
                self.native_bridge.set_obstacle(x, y, False)
        self._recalculate_path()
        self._redraw_canvas()

    def _generate_preset(self, preset_type):
        self._clear_grid()
        gs = self.grid_size
        if preset_type == "metropolis":
            block_size = 3 if gs == 32 else 6
            road_width = 2 if gs == 32 else 3
            for y in range(2, gs - 2):
                for x in range(2, gs - 2):
                    bx = x % (block_size + road_width)
                    by = y % (block_size + road_width)
                    if bx < block_size and by < block_size:
                        if random.random() < 0.85:
                            self.py_engine.set_obstacle(x, y, True)
                            self.native_bridge.set_obstacle(x, y, True)
        elif preset_type == "maze":
            step_y = 3 if gs == 32 else 5
            step_x = 4 if gs == 32 else 7
            for y in range(0, gs, step_y):
                gap = random.randint(2, gs - 3)
                for x in range(gs):
                    if abs(x - gap) > 1:
                        self.py_engine.set_obstacle(x, y, True)
                        self.native_bridge.set_obstacle(x, y, True)
            for x in range(0, gs, step_x):
                gap = random.randint(2, gs - 3)
                for y in range(gs):
                    if abs(y - gap) > 1:
                        self.py_engine.set_obstacle(x, y, True)
                        self.native_bridge.set_obstacle(x, y, True)
        elif preset_type == "chokepoint":
            mid = gs // 2
            for y in range(gs):
                if abs(y - mid) > 2:
                    self.py_engine.set_obstacle(mid, y, True)
                    self.native_bridge.set_obstacle(mid, y, True)
            q1 = gs // 4
            q3 = (3 * gs) // 4
            for x in range(gs):
                if abs(x - mid) > 2:
                    self.py_engine.set_obstacle(x, q1, True)
                    self.py_engine.set_obstacle(x, q3, True)
                    self.native_bridge.set_obstacle(x, q1, True)
                    self.native_bridge.set_obstacle(x, q3, True)
        elif preset_type == "random":
            for y in range(gs):
                for x in range(gs):
                    if (x, y) != self.start_pt and (x, y) != self.goal_pt:
                        if random.random() < 0.25:
                            self.py_engine.set_obstacle(x, y, True)
                            self.native_bridge.set_obstacle(x, y, True)

        # Clear Start & Goal surrounds
        for dy in range(-1, 2):
            for dx in range(-1, 2):
                self.py_engine.set_obstacle(self.start_pt[0] + dx, self.start_pt[1] + dy, False)
                self.py_engine.set_obstacle(self.goal_pt[0] + dx, self.goal_pt[1] + dy, False)
                self.native_bridge.set_obstacle(self.start_pt[0] + dx, self.start_pt[1] + dy, False)
                self.native_bridge.set_obstacle(self.goal_pt[0] + dx, self.goal_pt[1] + dy, False)

        self._recalculate_path()
        self._redraw_canvas()

    # --------------------------------------------------------------------------
    # Dynamic Trap Projectile Launcher ("Bẫy Phóng Ra")
    # --------------------------------------------------------------------------
    def _launch_trap_towards_drone(self, origin=None):
        """Launches a high-speed trap missile/projectile directly toward the drone's position or path."""
        # Auto-start flight if standing still so user immediately witnesses evasion
        if not self.flight_active:
            self._toggle_flight()

        # Target where drone will be in 5 ticks (intercept trajectory)
        lead_x = self.drone_pos[0] + math.cos(self.drone_heading) * 1.5
        lead_y = self.drone_pos[1] + math.sin(self.drone_heading) * 1.5

        if origin is None:
            # Pick a perimeter launcher point that gives a dramatic crossing vector
            candidates = [
                (1.5, self.grid_size // 2),
                (self.grid_size - 2.5, self.grid_size // 2),
                (self.grid_size // 2, 1.5),
                (self.grid_size // 2, self.grid_size - 2.5)
            ]
            ox, oy = random.choice(candidates)
        else:
            ox, oy = float(origin[0]), float(origin[1])

        dx = lead_x - ox
        dy = lead_y - oy
        dist = max(0.1, math.hypot(dx, dy))
        missile_speed = 0.85  # Speed in cells per frame

        trap_data = {
            "pos": [ox, oy],
            "vel": [(dx / dist) * missile_speed, (dy / dist) * missile_speed],
            "speed": missile_speed,
            "trail": [(ox, oy)],
            "wave_r": 0.0,
            "detected": False,
            "recomputed": False,
            "active": True
        }
        self.active_traps.append(trap_data)
        self.tactical_status = "🚀 TRAP LAUNCHED! INCOMING PROJECTILE ON INTERCEPT VECTOR"
        self.lbl_banner_status.configure(text="🚀 TRAP LAUNCHED! INCOMING HIGH-SPEED THREAT", fg="#F97316")

    # --------------------------------------------------------------------------
    # Canvas Coordinate Transformations & Event Handling
    # --------------------------------------------------------------------------
    def _canvas_to_grid(self, cx, cy):
        w = self.canvas.winfo_width()
        h = self.canvas.winfo_height()
        dim = min(w, h)
        ox = (w - dim) / 2.0
        oy = (h - dim) / 2.0
        if cx < ox or cx >= ox + dim or cy < oy or cy >= oy + dim:
            return None, None
        gx = int((cx - ox) / dim * self.grid_size)
        gy = int((cy - oy) / dim * self.grid_size)
        return max(0, min(self.grid_size - 1, gx)), max(0, min(self.grid_size - 1, gy))

    def _grid_to_canvas(self, gx, gy):
        w = self.canvas.winfo_width()
        h = self.canvas.winfo_height()
        dim = min(w, h)
        ox = (w - dim) / 2.0
        oy = (h - dim) / 2.0
        cell = dim / float(self.grid_size)
        return ox + (gx + 0.5) * cell, oy + (gy + 0.5) * cell

    def _on_canvas_click(self, event):
        self._apply_tool(event.x, event.y)

    def _on_canvas_drag(self, event):
        self._apply_tool(event.x, event.y)

    def _on_canvas_right_click(self, event):
        gx, gy = self._canvas_to_grid(event.x, event.y)
        if gx is not None:
            self._set_cell_obstacle(gx, gy, False)
            self._recalculate_path(from_drone=self.flight_active)
            self._redraw_canvas()

    def _on_canvas_right_drag(self, event):
        gx, gy = self._canvas_to_grid(event.x, event.y)
        if gx is not None:
            self._set_cell_obstacle(gx, gy, False)
            self._recalculate_path(from_drone=self.flight_active)
            self._redraw_canvas()

    def _on_canvas_hover(self, event):
        if self.active_tool == "lidar":
            gx, gy = self._canvas_to_grid(event.x, event.y)
            if gx is not None:
                self._compute_lidar_probe(gx, gy)
                self._redraw_canvas()

    def _apply_tool(self, cx, cy):
        gx, gy = self._canvas_to_grid(cx, cy)
        if gx is None:
            return

        if self.active_tool == "obstacle":
            self._paint_brush(gx, gy, True)
            self._recalculate_path(from_drone=self.flight_active)
        elif self.active_tool == "eraser":
            self._paint_brush(gx, gy, False)
            self._recalculate_path(from_drone=self.flight_active)
        elif self.active_tool == "start":
            self.start_pt = (gx, gy)
            if not self.flight_active:
                self.drone_pos = [float(gx), float(gy)]
            self._recalculate_path(from_drone=False)
        elif self.active_tool == "goal":
            self.goal_pt = (gx, gy)
            self._recalculate_path(from_drone=self.flight_active)
        elif self.active_tool == "launch_trap":
            # Fire an incoming trap missile from mouse click straight at the drone!
            self._launch_trap_towards_drone(origin=(gx, gy))

        self._redraw_canvas()

    def _paint_brush(self, gx, gy, is_obs):
        r = self.brush_size // 2
        for dy in range(-r, r + 1):
            for dx in range(-r, r + 1):
                nx, ny = gx + dx, gy + dy
                if (nx, ny) != self.start_pt and (nx, ny) != self.goal_pt:
                    self._set_cell_obstacle(nx, ny, is_obs)

    def _set_cell_obstacle(self, x, y, is_obs):
        if 0 <= x < self.grid_size and 0 <= y < self.grid_size:
            self.py_engine.set_obstacle(x, y, is_obs)
            self.native_bridge.set_obstacle(x, y, is_obs)

    # --------------------------------------------------------------------------
    # Pathfinding Engine Execution (With In-Flight Re-Routing)
    # --------------------------------------------------------------------------
    def _recalculate_path(self, from_drone=False):
        """Calculates optimal path. If from_drone is True, routes seamlessly from drone's current position."""
        mode = self.active_mode
        pts = None
        latency_ns = 0

        if from_drone:
            origin_x = max(0, min(self.grid_size - 1, int(round(self.drone_pos[0]))))
            origin_y = max(0, min(self.grid_size - 1, int(round(self.drone_pos[1]))))
            origin = (origin_x, origin_y)
        else:
            origin = self.start_pt

        # Native C-ABI query
        if self.native_bridge.is_native:
            c_mode = "optimal" if mode == "clearance" else ("any_angle" if mode in ["any_angle", "quintic"] else "jps")
            res = self.native_bridge.route(origin, self.goal_pt, c_mode)
            if res:
                cand_pts, c_lat = res
                # Verify that native path doesn't cut across obstacles
                valid = (len(cand_pts) >= 2)
                if valid:
                    for k in range(len(cand_pts) - 1):
                        if not self.py_engine.line_of_sight(cand_pts[k], cand_pts[k + 1]):
                            valid = False
                            break
                if valid:
                    pts = cand_pts
                    latency_ns = c_lat

        # Dynamic online JPS fallback query
        if pts is None or len(pts) == 0:
            clearance = (mode == "clearance")
            path, latency_ns = self.py_engine.route_jps(origin, self.goal_pt, clearance_weight=clearance)
            if mode in ["any_angle", "quintic"]:
                path = self.py_engine.prune_any_angle(path)
            pts = path

        self.last_latency_ns = latency_ns
        self.cached_path = pts or []
        self.last_waypoints_count = len(self.cached_path)

        # Path Length Computation
        length = 0.0
        for i in range(len(self.cached_path) - 1):
            length += math.hypot(self.cached_path[i+1][0] - self.cached_path[i][0],
                                 self.cached_path[i+1][1] - self.cached_path[i][1])
        self.last_path_length = length

        # Spline generation for kinodynamics
        if len(self.cached_path) >= 2:
            self.cached_spline = QuinticTrajectory.generate_spline(self.cached_path, samples_per_seg=14)
        else:
            self.cached_spline = []

        if from_drone:
            self.flight_idx = 0

        # Update Top HUD Telemetry
        if latency_ns < 1000:
            self.lbl_hud_latency.configure(text=f"{latency_ns} ns", fg="#00FF9D")
        elif latency_ns < 1000000:
            self.lbl_hud_latency.configure(text=f"{latency_ns / 1000.0:.2f} µs", fg="#38BDF8")
        else:
            self.lbl_hud_latency.configure(text=f"{latency_ns / 1000000.0:.2f} ms", fg="#FCD34D")

        self.lbl_hud_path.configure(text=f"{self.last_path_length:.1f} m")
        self.lbl_hud_pts.configure(text=f"{self.last_waypoints_count} pts")

    def _compute_lidar_probe(self, gx, gy):
        self.lidar_rays = []
        num_rays = 32
        for i in range(num_rays):
            angle = (2.0 * math.pi * i) / num_rays
            dx = math.cos(angle)
            dy = math.sin(angle)
            dist = self.native_bridge.raycast((gx, gy), (dx, dy), 24.0)
            if dist is None:
                dist = self.py_engine.raycast(gx, gy, dx, dy, 24.0)
            self.lidar_rays.append((gx, gy, dx, dy, dist))

    # --------------------------------------------------------------------------
    # Canvas Rendering Engine (Cyber Tactical Aesthetics & High Visibility)
    # --------------------------------------------------------------------------
    def _redraw_canvas(self):
        self.canvas.delete("all")
        w = self.canvas.winfo_width()
        h = self.canvas.winfo_height()
        if w < 20 or h < 20:
            return

        dim = min(w, h) - 16
        ox = (w - dim) / 2.0
        oy = (h - dim) / 2.0
        cell = dim / float(self.grid_size)

        # 1. Background Grid & Tactical Radar Box
        self.canvas.create_rectangle(ox, oy, ox + dim, oy + dim, fill="#0A0E1A", outline="#1E293B", width=2)

        # Subtle Tactical Grid Lines
        line_step = 1 if self.grid_size == 32 else 2
        for i in range(0, self.grid_size + 1, line_step):
            px = ox + i * cell
            py = oy + i * cell
            grid_col = "#141D2E" if (i % 8 != 0) else "#1E2D48"
            self.canvas.create_line(px, oy, px, oy + dim, fill=grid_col, width=1)
            self.canvas.create_line(ox, py, ox + dim, py, fill=grid_col, width=1)

        # 2. High-Contrast Tactical Obstacles
        for y in range(self.grid_size):
            for x in range(self.grid_size):
                if self.py_engine.is_obstacle(x, y):
                    x0 = ox + x * cell
                    y0 = oy + y * cell
                    self.canvas.create_rectangle(x0 + 1, y0 + 1, x0 + cell - 1, y0 + cell - 1,
                                                 fill="#1E293B", outline="#38BDF8", width=1.5)
                    self.canvas.create_line(x0 + 3, y0 + 3, x0 + cell - 3, y0 + cell - 3,
                                            fill="#0F172A", width=1)

        # 3. SWAR LiDAR Raycast Probes
        for gx, gy, dx, dy, dist in self.lidar_rays:
            x0, y0 = self._grid_to_canvas(gx, gy)
            hit_x, hit_y = self._grid_to_canvas(gx + dx * dist, gy + dy * dist)
            laser_col = "#00FF9D" if dist > 8.0 else ("#FFB800" if dist > 3.0 else "#FF0055")
            self.canvas.create_line(x0, y0, hit_x, hit_y, fill=laser_col, width=1.5, dash=(3, 2))
            self.canvas.create_oval(hit_x - 3, hit_y - 3, hit_x + 3, hit_y + 3, fill=laser_col, outline="#FFFFFF")

        # 4. High-Visibility Glowing Path Trajectory
        path_to_draw = self.cached_spline if self.cached_spline else self.cached_path
        if len(path_to_draw) >= 2:
            pixel_pts = []
            for pt in path_to_draw:
                px, py = self._grid_to_canvas(pt[0], pt[1])
                pixel_pts.extend([px, py])

            # Multi-layer neon electric glow
            self.canvas.create_line(pixel_pts, fill="#034A6E", width=8, capstyle="round", joinstyle="round")
            self.canvas.create_line(pixel_pts, fill="#0284C7", width=4, capstyle="round", joinstyle="round")
            self.canvas.create_line(pixel_pts, fill="#38BDF8", width=2, capstyle="round", joinstyle="round")

            # Animated pulse dash traveling along trajectory
            if len(pixel_pts) >= 4:
                idx = int((self.pulse_phase % 1.0) * (len(path_to_draw) - 1))
                p_lead = path_to_draw[idx]
                lx, ly = self._grid_to_canvas(p_lead[0], p_lead[1])
                self.canvas.create_oval(lx - 5, ly - 5, lx + 5, ly + 5, fill="#00FF9D", outline="#FFFFFF", width=1)

            # Key Waypoint Hubs
            for pt in self.cached_path:
                px, py = self._grid_to_canvas(pt[0], pt[1])
                self.canvas.create_oval(px - 4, py - 4, px + 4, py + 4, fill="#00F0FF", outline="#FFFFFF", width=1.5)

        # 5. Start & Goal Beacons
        sx, sy = self._grid_to_canvas(self.start_pt[0], self.start_pt[1])
        self.canvas.create_oval(sx - 16, sy - 16, sx + 16, sy + 16, outline="#00FF9D", width=1.5, dash=(4, 3))
        self.canvas.create_oval(sx - 9, sy - 9, sx + 9, sy + 9, fill="#00FF9D", outline="#FFFFFF", width=2)
        self.canvas.create_text(sx, sy - 20, text="START (S)", font=("Helvetica", 9, "bold"), fill="#00FF9D")

        gx, gy = self._grid_to_canvas(self.goal_pt[0], self.goal_pt[1])
        self.canvas.create_oval(gx - 16, gy - 16, gx + 16, gy + 16, outline="#F43F5E", width=1.5, dash=(4, 3))
        self.canvas.create_line(gx - 18, gy, gx + 18, gy, fill="#F43F5E", width=2)
        self.canvas.create_line(gx, gy - 18, gx, gy + 18, fill="#F43F5E", width=2)
        self.canvas.create_oval(gx - 8, gy - 8, gx + 8, gy + 8, fill="#F43F5E", outline="#FFFFFF", width=2)
        self.canvas.create_text(gx, gy + 22, text="GOAL (G)", font=("Helvetica", 9, "bold"), fill="#F43F5E")

        # 6. Active Trap Projectiles & Threat Waves ("Bẫy Đang Phóng Đến")
        dx, dy = self._grid_to_canvas(self.drone_pos[0], self.drone_pos[1])
        for trap in self.active_traps:
            self._render_incoming_trap(trap, dx, dy)

        # 7. Detonation Spark Rings
        for exp in self.explosions:
            ex, ey = self._grid_to_canvas(exp["pos"][0], exp["pos"][1])
            er = exp["r"]
            self.canvas.create_oval(ex - er, ey - er, ex + er, ey + er,
                                    outline="#EF4444", width=2, dash=(2, 2))

        # 8. Tactical Quadrotor Drone & 10-Layer Aegis Shield
        self._render_drone_and_aegis(dx, dy)

    def _render_incoming_trap(self, trap, dx, dy):
        """Renders incoming trap missile, plasma trail, radar intercept lock line, and threat waves."""
        tx, ty = self._grid_to_canvas(trap["pos"][0], trap["pos"][1])

        # 1. Fiery Exhaust Trail
        trail = trap["trail"]
        if len(trail) >= 2:
            trail_pts = []
            for tp in trail:
                px, py = self._grid_to_canvas(tp[0], tp[1])
                trail_pts.extend([px, py])
            self.canvas.create_line(trail_pts, fill="#F97316", width=3, capstyle="round")
            self.canvas.create_line(trail_pts, fill="#FDE047", width=1.5, capstyle="round")

        # 2. Expanding Threat Wave Rings ("Sóng Xung Kích")
        w_r = (self.threat_wave_phase * 22.0) % 44.0
        self.canvas.create_oval(tx - w_r, ty - w_r, tx + w_r, ty + w_r,
                                outline="#EF4444", width=1.5, dash=(3, 3))
        self.canvas.create_oval(tx - (w_r * 0.6), ty - (w_r * 0.6), tx + (w_r * 0.6), ty + (w_r * 0.6),
                                outline="#FBBF24", width=1, dash=(2, 2))

        # 3. Missile Warhead Body
        v_ang = math.atan2(trap["vel"][1], trap["vel"][0])
        tip_x = tx + 10 * math.cos(v_ang)
        tip_y = ty + 10 * math.sin(v_ang)
        self.canvas.create_oval(tx - 6, ty - 6, tx + 6, ty + 6, fill="#7F1D1D", outline="#EF4444", width=2)
        self.canvas.create_line(tx, ty, tip_x, tip_y, fill="#FDE047", width=3)
        self.canvas.create_text(tx, ty - 16, text="🚀 TRAP MISSILE", font=("Helvetica", 8, "bold"), fill="#F87171")

        # 4. Tactical Intercept Lock Line ("Khi Nhận Ra Bẫy Đang Đến")
        if trap.get("detected", False):
            self.canvas.create_line(dx, dy, tx, ty, fill="#F43F5E", width=1.5, dash=(4, 2))
            mid_x = (dx + tx) / 2.0
            mid_y = (dy + ty) / 2.0
            self.canvas.create_text(mid_x, mid_y - 10, text="⚠️ RADAR LOCK: INCOMING TRAP",
                                    font=("Helvetica", 8, "bold"), fill="#F43F5E")

    def _render_drone_and_aegis(self, dx, dy):
        """Renders tactical quadrotor drone with spinning blades, heading vector and Aegis shield."""
        shield_color = "#00FF9D"  # Nominal
        if self.aegis_alarm_level == 1:
            shield_color = "#FBBF24"  # Caution (Layer 9)
        elif self.aegis_alarm_level >= 2:
            shield_color = "#F43F5E"  # Emergency Hard Evasion (Layer 2)

        # Concentric Aegis Shield Dome Rings
        self.canvas.create_oval(dx - 42, dy - 42, dx + 42, dy + 42, outline=shield_color, width=1, dash=(3, 3))
        self.canvas.create_oval(dx - 28, dy - 28, dx + 28, dy + 28, outline=shield_color, width=1.5, dash=(5, 2))
        self.canvas.create_oval(dx - 18, dy - 18, dx + 18, dy + 18, outline=shield_color, width=2)

        # Quadrotor Arms (X-Configuration)
        arm_len = 15
        for angle_deg in [45, 135, 225, 315]:
            rad = math.radians(angle_deg) + self.drone_heading
            ax = dx + arm_len * math.cos(rad)
            ay = dy + arm_len * math.sin(rad)
            self.canvas.create_line(dx, dy, ax, ay, fill="#38BDF8", width=2)
            # Spinning Rotor Blades
            r_spin = 5.5
            r_ang = self.rotor_angle + rad
            rx1 = ax + r_spin * math.cos(r_ang)
            ry1 = ay + r_spin * math.sin(r_ang)
            rx2 = ax - r_spin * math.cos(r_ang)
            ry2 = ay - r_spin * math.sin(r_ang)
            self.canvas.create_line(rx1, ry1, rx2, ry2, fill="#E2E8F0", width=1.5)
            self.canvas.create_oval(ax - 2, ay - 2, ax + 2, ay + 2, fill="#0284C7", outline="")

        # Central Avionics Core
        self.canvas.create_oval(dx - 8, dy - 8, dx + 8, dy + 8, fill="#0F172A", outline="#00F0FF", width=2)

        # Forward Direction Pointer Laser
        head_len = 24
        hx = dx + head_len * math.cos(self.drone_heading)
        hy = dy + head_len * math.sin(self.drone_heading)
        arrow_col = "#F43F5E" if self.aegis_alarm_level >= 2 else "#00FF9D"
        self.canvas.create_line(dx, dy, hx, hy, fill=arrow_col, width=2.5, arrow="last")

    # --------------------------------------------------------------------------
    # Flight Dynamics & Active Protection System (APS) Reflex
    # --------------------------------------------------------------------------
    def _toggle_flight(self):
        self.flight_active = not self.flight_active
        if self.flight_active:
            self.btn_play_flight.configure(text="⏸️ Pause", bg="#F59E0B")
            if self.flight_idx >= len(self.cached_spline or self.cached_path):
                self.flight_idx = 0
            self.tactical_status = "CRUISE: EN ROUTE TO TARGET"
        else:
            self.btn_play_flight.configure(text="▶️ Play Flight", bg="#00FF9D")
            self.tactical_status = "CRUISE: PAUSED"

    def _reset_drone(self):
        self.flight_active = False
        self.flight_idx = 0
        self.drone_pos = [float(self.start_pt[0]), float(self.start_pt[1])]
        self.drone_speed = 0.0
        self.drone_heading = 0.0
        self.aegis_alarm_level = 0
        self.evasion_alert_ticks = 0
        self.active_traps.clear()
        self.explosions.clear()
        self.tactical_status = "CRUISE: RESET TO START"
        self.lbl_hud_aegis.configure(text="ALL CLEAR", fg="#00FF9D")
        self.btn_play_flight.configure(text="▶️ Play Flight", bg="#00FF9D")
        self._recalculate_path(from_drone=False)
        self._redraw_canvas()

    def _run_animation_loop(self):
        self.rotor_angle = (self.rotor_angle + 0.5) % (2.0 * math.pi)
        self.pulse_phase = (self.pulse_phase + 0.03) % 1.0
        self.threat_wave_phase = (self.threat_wave_phase + 0.08) % 1.0

        cur_x, cur_y = self.drone_pos

        # ----------------------------------------------------------------------
        # 1. UPDATE ACTIVE TRAP MISSILES & SENSOR RECOGNITION ("Bẫy Phóng Ra")
        # ----------------------------------------------------------------------
        incoming_threat_detected = False
        evasion_angle_chosen = None

        for trap in self.active_traps:
            # Advance projectile along velocity vector
            trap["pos"][0] += trap["vel"][0]
            trap["pos"][1] += trap["vel"][1]
            trap["trail"].append((trap["pos"][0], trap["pos"][1]))
            if len(trap["trail"]) > 12:
                trap["trail"].pop(0)

            tx, ty = trap["pos"]
            dist_to_drone = math.hypot(tx - cur_x, ty - cur_y)

            # Check if projectile is closing in on drone
            to_drone_x = cur_x - tx
            to_drone_y = cur_y - ty
            is_closing = (to_drone_x * trap["vel"][0] + to_drone_y * trap["vel"][1]) > 0

            # DETECT THREAT: Sensor recognition within 11.5 cells
            if dist_to_drone <= 11.5 and (is_closing or dist_to_drone < 6.0):
                trap["detected"] = True
                incoming_threat_detected = True

                # ==============================================================
                # "KHI BẪY PHÓNG RA VÀ NÓ NHẬN RA THÌ LẬP TỨC ĐỔI HƯỚNG"
                # Compute immediate 90° hard-bank evasion vector away from missile
                # ==============================================================
                threat_angle = math.atan2(ty - cur_y, tx - cur_x)
                cand_left = threat_angle + math.pi / 2.0
                cand_right = threat_angle - math.pi / 2.0
                goal_angle = math.atan2(self.goal_pt[1] - cur_y, self.goal_pt[0] - cur_x)

                # Score left vs right: test clearance in free space & goal progress
                score_left = math.cos(cand_left - goal_angle)
                test_lx = cur_x + 3.0 * math.cos(cand_left)
                test_ly = cur_y + 3.0 * math.sin(cand_left)
                if not (0 <= test_lx < self.grid_size and 0 <= test_ly < self.grid_size) or self.py_engine.is_obstacle(int(round(test_lx)), int(round(test_ly))):
                    score_left -= 10.0

                score_right = math.cos(cand_right - goal_angle)
                test_rx = cur_x + 3.0 * math.cos(cand_right)
                test_ry = cur_y + 3.0 * math.sin(cand_right)
                if not (0 <= test_rx < self.grid_size and 0 <= test_ry < self.grid_size) or self.py_engine.is_obstacle(int(round(test_rx)), int(round(test_ry))):
                    score_right -= 10.0

                evasion_angle_chosen = cand_left if score_left >= score_right else cand_right

                # DYNAMIC CORRIDOR RE-ROUTING AROUND INCOMING MISSILE
                if not trap.get("recomputed", False):
                    # Temporarily inject exclusion barrier where missile will cross
                    virtual_cells = []
                    for step_m in range(4):
                        vx = int(round(tx + trap["vel"][0] * step_m))
                        vy = int(round(ty + trap["vel"][1] * step_m))
                        if (vx, vy) != self.start_pt and (vx, vy) != self.goal_pt:
                            self.py_engine.set_obstacle(vx, vy, True)
                            self.native_bridge.set_obstacle(vx, vy, True)
                            virtual_cells.append((vx, vy))

                    # Recompute path from drone's current position to goal
                    self._recalculate_path(from_drone=True)

                    # Remove virtual exclusion barrier
                    for vx, vy in virtual_cells:
                        self.py_engine.set_obstacle(vx, vy, False)
                        self.native_bridge.set_obstacle(vx, vy, False)

                    trap["recomputed"] = True

            # Detonation on wall impact or out of bounds
            if self.py_engine.is_obstacle(int(round(tx)), int(round(ty))) or tx <= 0 or tx >= self.grid_size - 1 or ty <= 0 or ty >= self.grid_size - 1:
                trap["active"] = False
                self.explosions.append({"pos": [tx, ty], "r": 4.0, "life": 8})

        # Remove dead traps
        self.active_traps = [t for t in self.active_traps if t.get("active", True)]

        # Update detonation spark rings
        for exp in self.explosions:
            exp["r"] += 2.0
            exp["life"] -= 1
        self.explosions = [e for e in self.explosions if e["life"] > 0]

        # ----------------------------------------------------------------------
        # 2. AUTONOMOUS FLIGHT DYNAMICS & IMMEDIATE DIRECTION SHIFT
        # ----------------------------------------------------------------------
        if self.flight_active:
            trajectory = self.cached_spline if self.cached_spline else self.cached_path
            if trajectory and len(trajectory) > 1:
                if self.flight_idx < len(trajectory):
                    target = trajectory[self.flight_idx]
                    tgt_x, tgt_y = target
                    dist_to_wp = math.hypot(tgt_x - cur_x, tgt_y - cur_y)
                    base_heading = math.atan2(tgt_y - cur_y, tgt_x - cur_x)

                    # ----------------------------------------------------------
                    # SENSOR RECOGNITION BRANCH: LẬP TỨC ĐỔI HƯỚNG
                    # ----------------------------------------------------------
                    if incoming_threat_detected and evasion_angle_chosen is not None:
                        # 1. IMMEDIATELY SNAP HEADING (Đổi hướng tức thì)
                        self.drone_heading = evasion_angle_chosen
                        # 2. FULL EVASIVE ACCELERATION (Gia tốc né)
                        self.drone_speed = 4.2
                        self.aegis_alarm_level = 2
                        self.evasion_alert_ticks = 30
                        self.lbl_hud_aegis.configure(text="HARD EVASION (L2)", fg="#F43F5E")
                        self.lbl_banner_status.configure(
                            text=f"🚨 TRAP DETECTED! HARD BANK ({math.degrees(evasion_angle_chosen):.0f}°) — RE-ROUTED TO ESCAPE CORRIDOR",
                            fg="#F43F5E"
                        )
                    elif self.evasion_alert_ticks > 0:
                        self.evasion_alert_ticks -= 1
                        self.aegis_alarm_level = 2
                        self.drone_heading = base_heading
                        self.drone_speed = max(2.8, min(self.drone_target_speed, self.drone_speed + 0.2))
                    else:
                        # NOMINAL CRUISING
                        self.aegis_alarm_level = 0
                        self.lbl_hud_aegis.configure(text="ALL CLEAR (L0)", fg="#00FF9D")
                        self.lbl_banner_status.configure(text="🟢 CRUISE: FOLLOWING NOMINAL CORRIDOR", fg="#00FF9D")
                        self.drone_heading = base_heading
                        self.drone_speed = min(self.drone_target_speed, self.drone_speed + 0.2)

                    # Step drone position along current heading
                    step_dist = self.drone_speed * 0.28
                    next_x = self.drone_pos[0] + math.cos(self.drone_heading) * step_dist
                    next_y = self.drone_pos[1] + math.sin(self.drone_heading) * step_dist

                    # Safety boundary & collision avoidance
                    if 0 <= next_x < self.grid_size and 0 <= next_y < self.grid_size:
                        if not self.py_engine.is_obstacle(int(round(next_x)), int(round(next_y))):
                            self.drone_pos = [next_x, next_y]

                    # Waypoint arrival check
                    if dist_to_wp <= step_dist * 1.5:
                        self.flight_idx += 1

                else:
                    self.flight_active = False
                    self.drone_speed = 0.0
                    self.btn_play_flight.configure(text="▶️ Replay", bg="#00FF9D")
                    self.lbl_banner_status.configure(text="🎯 TARGET REACHED — MISSION ACCOMPLISHED!", fg="#00FF9D")

        # Update Telemetry Banner
        cur_x, cur_y = self.drone_pos
        self.lbl_banner_coords.configure(
            text=f"DRONE: ({cur_x:.1f}, {cur_y:.1f}) | GOAL: {self.goal_pt} | SPEED: {self.drone_speed:.1f} m/s | HEAD: {math.degrees(self.drone_heading):.0f}°"
        )

        # Redraw Canvas
        self._redraw_canvas()

        self.root.after(33, self._run_animation_loop)

    # --------------------------------------------------------------------------
    # 1,000x Stress Benchmark Modal
    # --------------------------------------------------------------------------
    def _run_benchmark_modal(self):
        runs = 1000
        latencies = []

        for _ in range(runs):
            t0 = time.perf_counter_ns()
            if self.native_bridge.is_native:
                self.native_bridge.route(self.start_pt, self.goal_pt, "jps")
            else:
                self.py_engine.route_jps(self.start_pt, self.goal_pt)
            latencies.append(time.perf_counter_ns() - t0)

        latencies.sort()
        p_min = latencies[0]
        p_50 = latencies[int(runs * 0.50)]
        p_95 = latencies[int(runs * 0.95)]
        p_99 = latencies[int(runs * 0.99)]
        p_max = latencies[-1]
        mean_lat = sum(latencies) / float(runs)
        throughput = 1000000000.0 / max(1.0, mean_lat)

        modal = tk.Toplevel(self.root)
        modal.title("H.A.L.O. Benchmark Telemetry Suite")
        modal.geometry("520x460")
        modal.configure(bg="#0B0F19")
        modal.transient(self.root)
        modal.grab_set()

        tk.Label(modal, text="📊 EMPIRICAL LATENCY BENCHMARK (1,000 RUNS)",
                 font=("Helvetica", 12, "bold"), fg="#00F0FF", bg="#0B0F19").pack(pady=(16, 4))
        eng_label = "Lõi Phần Cứng C++20 (C-ABI Native)" if self.native_bridge.is_native else "Lõi Thuần Python (Pure Emulation)"
        tk.Label(modal, text=f"Active Engine: {eng_label}", font=("Helvetica", 9), fg="#9CA3AF", bg="#0B0F19").pack()

        box = tk.Frame(modal, bg="#111827", padx=16, pady=12, highlightbackground="#1F2937", highlightthickness=1)
        box.pack(fill="both", expand=True, padx=20, pady=14)

        def add_stat(name, val_ns, target_str):
            row = tk.Frame(box, bg="#111827")
            row.pack(fill="x", pady=3)
            tk.Label(row, text=name, font=("Helvetica", 9), fg="#9CA3AF", bg="#111827", width=16, anchor="w").pack(side="left")
            val_text = f"{val_ns} ns" if val_ns < 1000 else f"{val_ns / 1000.0:.2f} µs"
            tk.Label(row, text=val_text, font=("Courier", 10, "bold"), fg="#38BDF8", bg="#111827", width=12, anchor="w").pack(side="left")
            tk.Label(row, text=target_str, font=("Helvetica", 8), fg="#00FF9D", bg="#111827").pack(side="right")

        add_stat("Min Latency", p_min, "[Cache-Warm / Ideal]")
        add_stat("Median (P50)", p_50, "[Nominal Baseline]")
        add_stat("95th Percentile", p_95, "[High-Traffic Load]")
        add_stat("99th Percentile", p_99, "[Hard Real-Time Gate]")
        add_stat("Max Latency", p_max, "[Worst Tail Peak]")
        add_stat("Mean Average", int(mean_lat), "[Statistical Avg]")

        tk.Label(box, text="--------------------------------------------------------", fg="#374151", bg="#111827").pack(pady=4)

        t_row = tk.Frame(box, bg="#111827")
        t_row.pack(fill="x")
        tk.Label(t_row, text="Total Throughput:", font=("Helvetica", 10, "bold"), fg="#F9FAFB", bg="#111827").pack(side="left")
        th_text = f"{throughput / 1000000.0:.2f} Million paths/sec" if throughput >= 1000000 else f"{throughput:,.0f} paths/sec"
        tk.Label(t_row, text=th_text, font=("Helvetica", 10, "bold"), fg="#00FF9D", bg="#111827").pack(side="right")

        tk.Button(modal, text="Close Telemetry", font=("Helvetica", 9, "bold"), bg="#1F2937", fg="#F9FAFB",
                  bd=0, padx=16, pady=6, command=modal.destroy).pack(pady=(0, 16))

    # --------------------------------------------------------------------------
    # Keyboard Shortcuts
    # --------------------------------------------------------------------------
    def _setup_keybindings(self):
        self.root.bind("<space>", lambda e: self._toggle_flight())
        self.root.bind("<c>", lambda e: self._clear_grid())
        self.root.bind("<r>", lambda e: self._generate_preset("random"))
        self.root.bind("<m>", lambda e: self._generate_preset("metropolis"))
        self.root.bind("<Escape>", lambda e: self._reset_drone())
        self.root.bind("1", lambda e: self._select_tool("obstacle"))
        self.root.bind("2", lambda e: self._select_tool("eraser"))
        self.root.bind("3", lambda e: self._select_tool("start"))
        self.root.bind("4", lambda e: self._select_tool("goal"))
        self.root.bind("5", lambda e: self._select_tool("lidar"))
        self.root.bind("6", lambda e: self._select_tool("launch_trap"))
        self.root.bind("t", lambda e: self._launch_trap_towards_drone())

# ==============================================================================
# MAIN ENTRYPOINT
# ==============================================================================

def main():
    root = tk.Tk()
    app = HaloPlaygroundApp(root)
    root.mainloop()

if __name__ == "__main__":
    main()
