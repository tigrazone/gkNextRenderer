// kit_raiden: NextRaiden low-poly brick arcade shooter asset kit.
// Pure module/function library, no top-level geometry, no textures.
// 1 unit = 1 m; Z-up; ALL aircraft / guns / boats face -Y, matching ScadRig.
// Ground objects bottom z=0. Tiles TOP z=0, bottom=-0.4 (intentional zMin).
// Aircraft bottom z=0; flight altitude belongs to the caller. No baked shadows.
// Player width 5.5m, scouts 3.3m, boss 14m. Suggested portrait playfield: 28x44m.
// damage=0 intact, 1 stripped outer panels, 2 core only, 3 empty/destroyed.
// explode=0 assembled; >0 exploded inspection pose (NOT a physics simulation).
// raiden_airframes.scad owns stable per-instance IDs; raiden_parts.scad owns meshes.
// Each nr_chunk keeps id/releaseStage/massKg/hitPoints/group/collider metadata.
// Suggested convex proxies should be recentered at bounds centre on physics handoff.
// No preattached PhysicsComponents: future application owns hit routing, detachment,
// world-pose preservation, velocities, pooling, lifetime and debris collision layers.
// For animated variants use characters/next_raiden_{rotor,gunship,fortress}.scad.
// Static SCAD previews show bind poses; clips are consumed only by ScadRig.

use <raiden_parts.scad>
use <raiden_airframes.scad>

// ================= Aircraft =================
module nr_veh_player(damage = 0, explode = 0) { nr_assemble(nr_interceptor(), 0, damage, explode); }
module nr_veh_wingman(damage = 0, explode = 0) { nr_assemble(nr_scout(), 0, damage, explode); }
module nr_veh_scout(paint = 1, damage = 0, explode = 0) { nr_assemble(nr_scout(), paint, damage, explode); }
module nr_veh_interceptor(paint = 1, damage = 0, explode = 0) { nr_assemble(nr_interceptor(), paint, damage, explode); }
module nr_veh_bomber(paint = 2, damage = 0, explode = 0) { nr_assemble(nr_bomber(), paint, damage, explode); }
module nr_veh_rotorcraft(paint = 3, damage = 0, explode = 0) { nr_assemble(nr_rotorcraft(), paint, damage, explode); }
module nr_veh_gunship(paint = 4, damage = 0, explode = 0) { nr_assemble(nr_gunship(), paint, damage, explode); }
module nr_veh_fortress(paint = 1, damage = 0, explode = 0) { nr_assemble(nr_fortress(), paint, damage, explode); }

// ================= Individual detachable pieces =================
module nr_part_armour(paint = 1) { nr_chunk(size = [1.1,1.2,0.28], paint = paint); }
module nr_part_wing(paint = 1) { nr_chunk(id = "wing", size = [1.1,1.9,0.23], shape = 1, paint = paint); }
module nr_part_canopy() { nr_chunk(id = "canopy", size = [0.8,1.4,0.6], shape = 2); }
module nr_part_engine() { nr_chunk(id = "engine", size = [0.7,1.5,0.8], shape = 3, tone = 3, releaseStage = 2); }
module nr_part_core() { nr_chunk(id = "core", size = [1.0,1.5,0.6], shape = 4, releaseStage = 3); }
module nr_part_gun() { nr_chunk(id = "gun", size = [0.4,1.6,0.4], shape = 5, releaseStage = 2); }
module nr_part_fin(paint = 1) { nr_chunk(id = "fin", size = [0.22,1.1,0.9], shape = 6, paint = paint); }
module nr_part_blade() { nr_chunk(id = "blade", size = [2.5,0.28,0.12], shape = 7, group = "rotor"); }
module nr_part_pod(paint = 1) { nr_chunk(id = "pod", size = [0.7,1.1,0.5], shape = 8, paint = paint); }

// Resting wreck is assembled from the SAME chunks, not a separate replacement mesh.
// Flat Z placement intentionally preserves reliable ground contact without physics.
module nr_prop_wreck(seed = 4, paint = 1, stage = 1)
{
    parts = nr_interceptor();
    for (i = [0:len(parts)-1])
        let (r = parts[i])
            if (r[5] <= stage)
                translate([(i%5-2)*1.65, (floor(i/5)-1.5)*2.35, 0])
                    rotate([0,0,nr_rnd(seed+i*17,360)]) nr_piece(r, paint);
}

// ================= Ground enemies and boats =================
module nr_veh_tank(paint = 2, damage = 0)
{
    if (damage < 3) nr_chunk(id = "chassis", size = [1.9,3.1,0.60], shape = 4, releaseStage = 3, paint = paint);
    if (damage < 2)
    {
        for (s = [-1,1]) translate([s*1.15,0,0])
            nr_chunk(id = str("track_",s), size = [0.50,3.35,0.70], tone = 2, releaseStage = 2);
        translate([0,0.24,0.68]) nr_chunk(id = "turret", size = [1.38,1.55,0.6], paint = paint, releaseStage = 2);
        translate([0,-1.0,0.93]) nr_chunk(id = "barrel", size = [0.35,2.5,0.35], shape = 5, releaseStage = 2);
    }
    if (damage < 1) for (s = [-1,1]) translate([s*0.82,0,0.68])
        nr_chunk(id = str("skirt_",s), size = [0.29,2.8,0.30], paint = paint);
}

module nr_prop_turret(paint = 1, damage = 0)
{
    if (damage < 3) nr_chunk(id = "base", size = [2.3,2.3,0.38], shape = 4, releaseStage = 3);
    if (damage < 2)
    {
        translate([0,0,0.59]) nr_chunk(id = "head", size = [1.65,1.6,0.7], paint = paint, releaseStage = 2);
        for (s = [-1,1]) translate([s*0.44,-0.70,1.0])
            nr_chunk(id = str("barrel_",s), size = [0.3,1.75,0.34], shape = 5, releaseStage = 2);
    }
    if (damage < 1) for (s = [-1,1]) translate([s*0.96,0,0.39])
        nr_chunk(id = str("shield_",s), size = [0.3,1.9,0.95], paint = paint);
}

module nr_veh_launcher(paint = 2, damage = 0)
{
    if (damage < 3) nr_chunk(id = "chassis", size = [1.8,3.7,0.56], shape = 4, releaseStage = 3);
    if (damage < 2)
    {
        translate([0,-1.22,0.58]) nr_chunk(id = "cab", size = [1.75,1.1,0.9], shape = 2);
        for (s = [-1,1]) translate([s*1.02,0,0])
            nr_chunk(id = str("track_",s), size = [0.4,3.6,0.58], tone = 2, releaseStage = 2);
    }
    if (damage < 1) for (s = [-1,1], y = [0,1]) translate([s*0.47,y*0.95,0.78])
        nr_chunk(id = str("rack_",s,"_",y), size = [0.84,0.86,0.75], shape = 8, paint = paint);
}

module nr_veh_patrolboat(paint = 2, damage = 0)
{
    if (damage < 3) nr_chunk(id = "keel", size = [1.3,5.4,0.45], shape = 4, releaseStage = 3);
    if (damage < 2)
    {
        for (s = [-1,1], j = [0:2]) translate([s*1.0,-1.8+j*1.8,0.08])
            nr_chunk(id = str("hull_",s,"_",j), size = [0.65,1.72,0.65], paint = paint, releaseStage = 2);
        translate([0,0.75,0.68]) nr_chunk(id = "bridge", size = [1.5,1.4,1.2], shape = 2);
        translate([0,-1.35,0.69]) nr_chunk(id = "deck_gun", size = [0.62,1.7,0.55], shape = 5, releaseStage = 2);
    }
    if (damage < 1) translate([0,1.8,0.56]) nr_chunk(id = "exhaust", size = [0.9,0.75,0.65], shape = 3);
}

// ================= Scrolling tiles: 16m pitch, top = 0 =================
module nr_ground_grass(seed = 0)
{
    gk_flatten()
    {
        color(nr_SOIL()) translate([0,0,-0.4]) nr_box([16,16,0.36]);
        color(nr_GRASS()) translate([0,0,-0.04]) nr_box([16,16,0.04]);
        for (i = [0:6]) color([0.24,0.37,0.17])
            translate([nr_rnd(seed+i*11,120)/10-6, nr_rnd(seed+i*29,120)/10-6, 0])
                nr_box([1.1+nr_rnd(i+seed,20)/10,0.6,0.012]);
    }
}
module nr_ground_runway()
{
    gk_flatten()
    {
        color(nr_DARK()) translate([0,0,-0.4]) nr_box([16,16,0.4]);
        for (s = [-1,1]) color(nr_WHITE()) translate([s*6.4,0,0]) nr_box([0.14,16,0.025]);
        for (y = [-6,-2,2,6]) color(nr_WHITE()) translate([0,y,0]) nr_box([0.26,1.65,0.025]);
        for (s = [-1,1], y = [-7,-3,1,5])
            color(nr_YELLOW()) translate([s*7.5,y,0]) nr_box([0.22,0.38,0.08]);
    }
}
module nr_ground_road()
{
    gk_flatten()
    {
        color(nr_GRASS()) translate([0,0,-0.4]) nr_box([16,16,0.4]);
        color([0.12,0.15,0.15]) translate([0,0,0.02]) nr_box([6,16,0.04]);
        for (s = [-1,1]) color(nr_CONCRETE()) translate([s*3.6,0,0.02]) nr_box([1.0,16,0.08]);
        for (y = [-6,-2,2,6]) color(nr_YELLOW()) translate([0,y,0.065]) nr_box([0.16,1.8,0.015]);
    }
}
module nr_ground_water()
{
    gk_flatten()
    {
        gk_material(nr_WATER(), roughness = 0.28) translate([0,0,-0.4]) nr_box([16,16,0.4]);
        for (i = [0:6]) color([0.065,0.32,0.35])
            translate([nr_rnd(i*7,120)/10-6,nr_rnd(i*13,120)/10-6,0]) nr_box([1.6,0.09,0.012]);
    }
}
module nr_ground_quay()
{
    gk_flatten()
    {
        color(nr_CONCRETE()) translate([0,0,-0.4]) nr_box([16,16,0.4]);
        for (x = [-7.5,7.5]) color(nr_YELLOW()) translate([x,0,0]) nr_box([0.18,16,0.03]);
        for (y = [-4,4]) color(nr_DARK()) translate([0,y,0]) nr_box([16,0.06,0.012]);
    }
}
module nr_ground_farmland()
{
    gk_flatten()
    {
        color(nr_SOIL()) translate([0,0,-0.4]) nr_box([16,16,0.4]);
        for (x = [-7:2:7]) color([0.30,0.39,0.12]) translate([x,0,0]) nr_bevel([0.85,15.5,0.20]);
    }
}

// ================= Buildings and scenery =================
module nr_bldg_hangar(paint = 2)
{
    gk_flatten()
    {
        color(nr_CONCRETE()) nr_box([7.0,7.2,0.20]);
        for (s = [-1,1]) nr_surface(nr_WHITE()) translate([s*3.12,0,0.2]) nr_box([0.48,6.8,2.65]);
        nr_surface(nr_WHITE()) translate([0,3.1,0.2]) nr_box([6.4,0.48,2.65]);
        nr_surface(nr_paint(paint)) translate([0,0,2.85]) nr_bevel([6.8,7.0,0.65]);
        for (x = [-2.6,0,2.6]) nr_surface(nr_YELLOW()) translate([x,-3.28,0.2]) nr_box([0.16,0.13,2.7]);
        for (y = [-2,0,2]) nr_surface(nr_STEEL()) translate([0,y,3.51]) nr_box([6.2,0.12,0.04]);
    }
}
module nr_bldg_controltower()
{
    gk_flatten()
    {
        color(nr_CONCRETE()) nr_bevel([3.4,3.4,0.35]);
        nr_surface(nr_WHITE()) translate([0,0,0.35]) nr_bevel([1.7,1.8,4.2]);
        nr_surface(nr_STEEL()) translate([0,0,4.55]) nr_bevel([3.1,2.8,0.28]);
        nr_surface(nr_GLASS()) translate([0,0,4.83]) nr_bevel([2.7,2.4,1.05]);
        nr_surface(nr_RED()) translate([0,0,5.88]) nr_bevel([3.25,2.96,0.32]);
        nr_surface(nr_WHITE()) translate([0,0,6.2]) cylinder(h = 1.3, r = 0.08, $fn = 6);
        nr_surface(nr_YELLOW()) translate([0,0,7.5]) cube([0.22,0.22,0.22], center = true);
    }
}
module nr_bldg_bunker(damage = 0)
{
    if (damage < 3) nr_chunk(id = "foundation", size = [4.8,4.2,0.45], shape = 4, releaseStage = 3);
    if (damage < 2) for (s = [-1,1]) translate([s*1.9,0,0.46])
        nr_chunk(id = str("wall_",s), size = [0.65,4.1,1.55], tone = 3, releaseStage = 2);
    if (damage < 1) for (i = [-1,0,1]) translate([i*1.5,0,2.05])
        nr_chunk(id = str("roof_",i), size = [1.4,4.3,0.48], paint = 2);
}
module nr_bldg_factory()
{
    gk_flatten()
    {
        nr_surface(nr_WHITE()) nr_bevel([6.0,7.0,2.8]);
        for (y = [-2.3,0,2.3]) nr_surface(nr_TEAL()) translate([0,y,2.8]) nr_bevel([6.4,2.18,0.65]);
        for (x = [-2,0,2]) nr_surface(nr_GLASS()) translate([x,-3.51,1.1]) nr_box([1.15,0.03,0.8]);
        for (x = [-1.8,1.8])
        {
            nr_surface(nr_STEEL()) translate([x,2.0,3.4]) nr_box([0.85,0.85,2.0]);
            nr_surface(nr_RED()) translate([x,2.0,5.4]) nr_box([0.88,0.88,0.35]);
        }
    }
}
module nr_bldg_fueldepot()
{
    gk_flatten()
    {
        color(nr_CONCRETE()) nr_box([6.5,5,0.25]);
        for (s = [-1,1])
        {
            nr_surface(nr_WHITE()) translate([s*1.65,0,0.25]) cylinder(h = 2.9, r = 1.23, $fn = 10);
            nr_surface(nr_RED()) translate([s*1.65,0,2.0]) cylinder(h = 0.40, r = 1.25, $fn = 10);
            nr_surface(nr_STEEL()) translate([s*1.65,0,3.15]) cylinder(h = 0.20, r1 = 1.23, r2 = 0.84, $fn = 10);
        }
    }
}
module nr_prop_radar()
{
    gk_flatten()
    {
        color(nr_CONCRETE()) nr_bevel([2.6,2.6,0.3]);
        nr_surface(nr_STEEL()) translate([0,0,0.3]) nr_box([0.42,0.42,2.0]);
        nr_surface(nr_TEAL()) translate([0,0,2.3]) rotate([15,0,0]) nr_bevel([3.0,0.22,1.45]);
        for (x = [-1,0,1]) nr_surface(nr_WHITE()) translate([x,-0.15,2.45]) nr_box([0.06,0.06,1.0]);
    }
}
module nr_prop_container(paint = 1)
{
    gk_flatten()
    {
        nr_surface(nr_paint(paint)) nr_bevel([2.4,5.0,2.3]);
        for (y = [-2:0.65:2]) nr_surface(nr_STEEL()) translate([0,y,2.3]) nr_box([2.15,0.07,0.04]);
        for (x = [-0.65,0.65]) nr_surface(nr_WHITE()) translate([x,-2.51,0.2]) nr_box([0.08,0.04,1.8]);
    }
}
module nr_prop_crane()
{
    gk_flatten()
    {
        color(nr_CONCRETE()) nr_box([4.2,4.2,0.3]);
        for (s = [-1,1]) nr_surface(nr_YELLOW()) translate([s*1.6,0,0.3]) nr_box([0.5,1.5,5.3]);
        nr_surface(nr_YELLOW()) translate([0,-1.4,5.6]) nr_box([4.0,6.0,0.65]);
        nr_surface(nr_DARK()) translate([0,-3.75,2.6]) nr_box([0.10,0.10,3.0]);
        nr_surface(nr_STEEL()) translate([0,-3.75,2.4]) nr_box([1.8,0.4,0.22]);
        nr_surface(nr_GLASS()) translate([1.05,-0.6,4.3]) nr_bevel([1.1,1.4,1.15]);
    }
}
module nr_prop_barrier()
{
    gk_flatten()
    {
        color(nr_CONCRETE()) nr_bevel([3.4,0.65,0.75]);
        for (i = [-3:3]) color(i%2==0 ? nr_YELLOW() : nr_DARK())
            translate([i*0.44,-0.331,0.28]) nr_box([0.42,0.025,0.25]);
    }
}
module nr_prop_crate()
{
    gk_flatten()
    {
        nr_surface(nr_TEAL()) nr_bevel([1.2,1.2,1.05]);
        for (s = [-1,1]) nr_surface(nr_WHITE()) translate([s*0.38,0,1.05]) nr_box([0.14,1.18,0.05]);
    }
}
module nr_nature_tree(seed = 0)
{
    gk_flatten()
    {
        color(nr_SOIL()) nr_box([0.38,0.38,1.1]);
        for (j = [0:2]) color([0.10+j*0.035,0.24+j*0.04,0.12]) translate([0,0,0.65+j*0.7])
            rotate([0,0,nr_rnd(seed,40)]) cylinder(h = 1.4, r1 = 1.2-j*0.25, r2 = 0.1, $fn = 5);
    }
}
module nr_nature_rock(seed = 0)
{
    gk_flatten() color(nr_CONCRETE()) rotate([0,0,nr_rnd(seed,90)])
        scale([1.3,1,1]) cylinder(h = 0.95, r1 = 1.0, r2 = 0.62, $fn = 5);
}

// ================= Solid-colour projectile meshes (no VFX logic) =================
module nr_wpn_bolt(hostile = false)
{
    gk_flatten()
    {
        nr_surface(hostile ? nr_RED() : nr_GLASS()) nr_bevel([0.20,0.95,0.18]);
        nr_surface(nr_WHITE()) translate([0,-0.16,0.18]) nr_box([0.08,0.58,0.025]);
    }
}
module nr_wpn_orb()
{
    gk_flatten()
    {
        nr_surface(nr_RED()) translate([0,0,0.24]) sphere(r = 0.24, $fn = 8);
        nr_surface(nr_YELLOW()) translate([0,-0.08,0.32]) nr_bevel([0.20,0.22,0.12]);
    }
}
module nr_wpn_needle()
{
    gk_flatten() nr_surface(nr_YELLOW()) linear_extrude(height = 0.12)
        polygon([[0,-0.70],[0.16,0.36],[0,0.23],[-0.16,0.36]]);
}
module nr_wpn_missile(paint = 0)
{
    gk_flatten()
    {
        nr_surface(nr_WHITE()) nr_bevel([0.28,1.26,0.28]);
        nr_surface(nr_paint(paint)) translate([0,-0.68,0]) nr_bevel([0.27,0.27,0.26]);
        for (s = [-1,1]) nr_surface(nr_paint(paint)) translate([s*0.23,0.4,0.08]) nr_bevel([0.22,0.43,0.09]);
    }
}
module nr_wpn_bomb()
{
    gk_flatten()
    {
        nr_surface(nr_STEEL()) nr_bevel([0.62,1.1,0.50]);
        nr_surface(nr_YELLOW()) translate([0,-0.38,0.50]) nr_box([0.52,0.16,0.035]);
        nr_surface(nr_RED()) translate([0,0.70,0.12]) nr_box([0.86,0.34,0.22]);
    }
}
module nr_wpn_laser(length = 5)
{
    gk_flatten()
    {
        nr_surface(nr_GLASS()) nr_bevel([0.30,length,0.22]);
        nr_surface(nr_WHITE()) translate([0,0,0.22]) nr_box([0.10,length-0.08,0.035]);
    }
}

// Pickups: top-view geometry symbols, no font or texture dependencies.
module nr_item_power()
{
    gk_flatten()
    {
        nr_surface(nr_RED()) nr_bevel([0.95,0.95,0.24]);
        nr_surface(nr_WHITE()) translate([0,0,0.24]) linear_extrude(height = 0.05)
            polygon([[-0.12,-0.34],[0.24,-0.34],[0.02,-0.03],[0.22,-0.03],[-0.23,0.36],[-0.07,0.04],[-0.24,0.04]]);
    }
}
module nr_item_medal()
{
    gk_flatten()
    {
        nr_surface(nr_YELLOW()) cylinder(h = 0.22, r = 0.52, $fn = 8);
        nr_surface(nr_WHITE()) translate([0,0,0.22]) linear_extrude(height = 0.06)
            polygon([for(i=[0:9]) [(i%2==0?0.34:0.16)*sin(i*36), (i%2==0?0.34:0.16)*cos(i*36)]]);
    }
}
module nr_item_repair()
{
    gk_flatten()
    {
        nr_surface(nr_TEAL()) nr_bevel([0.95,0.95,0.24]);
        nr_surface(nr_WHITE()) translate([0,0,0.24])
        { nr_box([0.60,0.18,0.06]); nr_box([0.18,0.60,0.06]); }
    }
}
module nr_item_bomb()
{
    gk_flatten()
    {
        nr_surface(nr_YELLOW()) nr_bevel([1.1,1.1,0.2]);
        translate([0,0,0.2]) scale([0.65,0.65,0.65]) nr_wpn_bomb();
    }
}
module nr_item_option()
{
    gk_flatten()
    {
        nr_surface(nr_BLUE()) nr_bevel([1.1,1.1,0.2]);
        nr_surface(nr_WHITE()) translate([0,0,0.2]) linear_extrude(height = 0.06)
            polygon([[0,-0.40],[0.40,0.3],[0,0.16],[-0.40,0.3]]);
    }
}

// ================= Mesh-only effect building blocks =================
module nr_fx_exhaust()
{
    gk_flatten()
    {
        nr_surface(nr_YELLOW()) linear_extrude(height = 0.12) polygon([[-0.22,-0.35],[0.22,-0.35],[0,0.9]]);
        nr_surface(nr_WHITE()) translate([0,0,0.12]) linear_extrude(height = 0.04)
            polygon([[-0.11,-0.32],[0.11,-0.32],[0,0.4]]);
    }
}
module nr_fx_flash()
{
    gk_flatten() nr_surface(nr_YELLOW()) linear_extrude(height = 0.15)
        polygon([for(i=[0:11]) [(i%2==0?0.7:0.22)*sin(i*30), (i%2==0?0.7:0.22)*cos(i*30)]]);
}
module nr_fx_smoke(seed = 0)
{
    gk_flatten()
    {
        for (i = [0:3]) color([0.12+i*0.025,0.13+i*0.025,0.13+i*0.025])
            translate([nr_rnd(seed+i*7,10)/20-0.25,0,0.4+i*0.44])
                scale([1,0.85,1]) sphere(r = 0.4+i*0.10, $fn = 6);
    }
}
module nr_fx_blast()
{
    gk_flatten()
    {
        for (i = [0:5]) nr_surface(i%2==0?nr_YELLOW():nr_RED())
            translate([cos(i*60)*0.52,sin(i*60)*0.52,0.45]) sphere(r = 0.45, $fn = 6);
        nr_surface(nr_WHITE()) translate([0,0,0.65]) sphere(r = 0.38, $fn = 6);
    }
}
module nr_fx_crater()
{
    gk_flatten()
    {
        color(nr_DARK()) cylinder(h = 0.018, r = 1.05, $fn = 10);
        for (i = [0:7]) color(nr_SOIL()) translate([cos(i*45)*1.05,sin(i*45)*1.05,0])
            rotate([0,0,i*45]) nr_bevel([0.48,0.38,0.17]);
    }
}
