// ============================================================================
// stage1_airfield.scad - NextRaiden 关卡 1「机场突入」。
//
// 关卡沿 +Y 生长；y=0 是开场，玩家出生在 y≈-14 附近朝 +Y 飞。滚动 7.5 m/s，
// 所以 y 坐标除以 7.5 就是它出现的关卡秒数——摆地面目标时按这个换算对波次。
//
// 这里放两类东西：
//   * 景观带（nr_band_*），一行一个 Node，纯背景；
//   * 地面目标（nr_veh_tank / nr_prop_turret / nr_veh_launcher / nr_bldg_*），
//     运行时由 StageIndex 按模块名索引，能开火、能被打、按 chunk 分级掉块。
// 建筑里只有 bunker 和 fueldepot 是目标，hangar / controltower / radar 是布景。
//
// 空中波次不在这里：那是时间线，写在 assets/configs/nextraiden/stage1_waves.json。
// ============================================================================

use <../../lib/kit_raiden.scad>
use <../../lib/raiden_parts.scad>
use <../../lib/gk_camera.scad>
use <nr_stage_common.scad>

$fn = 8;

// ---- 景观带 ----------------------------------------------------------------
// 覆盖到 endZ 再往前一个可见范围（约 27 m）为止，否则关卡末尾会露出背景色。
// 现在覆盖到 SCAD y=1264 = 引擎 z=-1264；stages.json 的 endZ 是 -1180。
nr_band_grass(y0 = -48, rows = 3, seed = 3, props = 1);    // 出生点身后，防止开场露白
nr_band_runway(y0 = 0, rows = 8, seed = 11);               // 0:00-0:17 跑道序章
nr_band_grass(y0 = 128, rows = 4, seed = 57, props = 1);   // 0:17-0:26 地面段
nr_band_road(y0 = 192, rows = 5, seed = 83);               // 0:26-0:37 公路车队
nr_band_farm(y0 = 272, rows = 4, seed = 117);              // 0:37-0:46 农地
nr_band_runway(y0 = 336, rows = 6, seed = 151);            // 0:46-1:00 侧跑道
nr_band_grass(y0 = 432, rows = 5, seed = 191, props = 1);  // 1:00-1:11 中BOSS 空域
nr_band_road(y0 = 512, rows = 5, seed = 233);              // 1:11-1:22 补给公路
nr_band_runway(y0 = 592, rows = 10, seed = 277);           // 1:22-1:44 机场核心
nr_band_grass(y0 = 752, rows = 4, seed = 331, props = 1);
nr_band_runway(y0 = 816, rows = 8, seed = 373);            // 1:53-2:10 主跑道
nr_band_farm(y0 = 944, rows = 5, seed = 419);
nr_band_runway(y0 = 1024, rows = 10, seed = 467);          // 2:17-2:40 BOSS 空域与收尾
nr_band_grass(y0 = 1184, rows = 5, seed = 521, props = 1);

// ---- 取景校准物 ------------------------------------------------------------
// 塔台在 +X、机库在 -X：截图里塔台必须出现在**右**边。SCAD +X = 屏幕右方是整套
// 朝向契约的地基（设计 5.1），所以关卡里留一对不对称的参照物，改相机时一眼能看出翻没翻。
translate([11, 50, 0.02]) nr_bldg_controltower();
translate([-11, 56, 0.02]) nr_bldg_hangar();

// ---- 地面目标 --------------------------------------------------------------
// 0:20 起：坦克纵队沿跑道边缘，教玩家"地面也要打"
translate([-7, 150, 0.02]) nr_veh_tank();
translate([7, 168, 0.02]) nr_veh_tank();
translate([-6, 186, 0.02]) nr_veh_tank();
for (y = [140, 160, 180]) translate([-11, y, 0.02]) nr_prop_barrier();

// 0:26 公路段：炮塔夹道
translate([-9, 206, 0.02]) nr_prop_turret();
translate([9, 224, 0.02]) nr_prop_turret();
translate([0, 244, 0.02]) nr_veh_tank();

// 0:37 农地：第一门发射车，掉落 power
translate([8, 286, 0.02]) nr_veh_launcher();
translate([-9, 300, 0.02]) nr_veh_tank();
for (s = [-1, 1], i = [0:2]) translate([s * (20 + i), 280 + i * 14, 0.02]) nr_nature_tree(seed = i * 7 + 3);

// 0:46 侧跑道：掩体群 + 雷达
translate([-10, 350, 0.02]) nr_bldg_bunker();
translate([10, 372, 0.02]) nr_bldg_bunker();
translate([13, 392, 0.02]) nr_prop_radar();
translate([-12, 404, 0.02]) nr_prop_turret();

// 1:11 补给公路：油库——打爆会连锁
translate([-10, 522, 0.02]) nr_bldg_fueldepot();
translate([-10, 536, 0.02]) nr_bldg_fueldepot();
translate([9, 548, 0.02]) nr_veh_launcher();
translate([0, 562, 0.02]) nr_veh_tank();

// 1:22 机场核心：机库、工厂、集装箱与密集防空
translate([-12, 602, 0.02]) nr_bldg_hangar(paint = 1);
translate([12, 618, 0.02]) nr_bldg_factory();
translate([-9, 634, 0.02]) nr_prop_turret();
translate([9, 650, 0.02]) nr_prop_turret();
translate([-11, 666, 0.02]) nr_veh_launcher();
for (i = [0:2]) translate([13, 676 + i * 6, 0.02]) nr_prop_container(paint = i);
translate([0, 690, 0.02]) nr_veh_tank();
translate([-13, 704, 0.02]) nr_bldg_bunker();
translate([13, 712, 0.02]) nr_bldg_controltower();
translate([-8, 726, 0.02]) nr_prop_turret();

// 1:53 主跑道：发射车阵地
translate([9, 830, 0.02]) nr_veh_launcher();
translate([-9, 846, 0.02]) nr_veh_launcher();
translate([0, 862, 0.02]) nr_prop_turret();
translate([-11, 878, 0.02]) nr_veh_tank();
translate([11, 894, 0.02]) nr_veh_tank();
for (y = [820, 850, 880, 910]) translate([-14, y, 0.02]) nr_prop_barrier();

// 2:06 油库群：最后的送分点，也是最后一次补给机会
translate([-10, 920, 0.02]) nr_bldg_fueldepot();
translate([-10, 934, 0.02]) nr_bldg_fueldepot();
translate([10, 930, 0.02]) nr_bldg_fueldepot();
translate([0, 952, 0.02]) nr_prop_turret();

// 2:17 BOSS 空域前的最后阻拦
translate([-12, 1032, 0.02]) nr_prop_turret();
translate([12, 1044, 0.02]) nr_prop_turret();
translate([0, 1060, 0.02]) nr_veh_launcher();
translate([-13, 1076, 0.02]) nr_bldg_bunker();
translate([13, 1088, 0.02]) nr_bldg_bunker();

// ---- 料架 ------------------------------------------------------------------
nr_rack();

// ---- 取景 ------------------------------------------------------------------
// 与游戏内相机同一套参数：目标在地面 (0, y, 0)，眼位 = 目标 + SCAD(0, -12.6, 47)，
// 即引擎空间的 (0, +47, +12.6)。俯角 15 度，竖直 FOV 50 度。
gk_camera_lookat([0, 27.4, 47], [0, 40, 0], name = "01_Gameplay_framing", fov = 50);
gk_camera_lookat([0, 627.4, 47], [0, 640, 0], name = "02_Airfield_core", fov = 50);
gk_camera_lookat([0, 927.4, 47], [0, 940, 0], name = "03_Fuel_farm", fov = 50);
