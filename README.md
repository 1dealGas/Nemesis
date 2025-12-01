### Don‘t Leave the World Your `Nemesis`.

1dealGas's official `Chart[Fumen]` Compiler & Viewer of `Aerials`

## Get Started

1. Open the application, click `Works` button to open the workspace folder.

2. Get the track `「Chronomica」by polysha` [here](https://masamunejp.bandcamp.com/album/polysha-soundcloud-freedl-works) (`wav` or `mp3` format).
   
   Let's use the filename `chronomica.wav` for example.
   
   *Additionally, `Nemesis` supports `Opus` format.*

3. Create a code file `[Addon]`, put our official Addon lines into the file, save it, and then restart the `Nemesis` application.
   
   *If succeeded, the title of the `Nemesis` window will be `Nemesis with Addon`.*

4. Create the `Chart[Fumen]` code file `chronomica.lua`, put our example Chart/Fumen lines into the file, and then save it.

5. Click `Args Input` button of the `Nemesis` Window, type `chronomica` (with no file extension), and then click `Load` button.

6. You are ready to do what you want since then.

## APIs of `Chart[Fumen]`

These APIs are stated `local` in the `Chart[Fumen]` header (see [`Viewer.script`](./Viewer.script)), so there is no need to add sentences like `local Wish = Wish`.

#### `Time {}`

```lua
--[ Examples: ]--
Time {                       -- For 4/4-only tracks
    Offset = 0,              -- Beat 0 starts from 0ms
    0, 170,                  -- Bar, BPM
    ···
}
Time {                       -- For tracks with Tempo Variations
    Offset = 1,              -- Offset must be positive
    Tempo = {
        0, 4, 4,             -- Bar, Beat Count of a Bar, Tone Divisor
        1, 3, 4,
        25, 4, 4
    },
    0, 0, 201,               -- Bar(to be converted to Beat), Additional Beats, BPM
    ···                      -- Negative BPM as Linear BPM
}
```

#### `Delta {}`

```lua
--[ Example: ]--
Delta {
    {0},        1,           -- Bar 0, Ratio: 1
    {2, 1/32},  -1,          -- Bar 2, then 1/32 Tone, Ratio: -1
    {2, -1},    0.9,         -- Bar 2, then 1/16 Tone, Ratio: 0.9
    -15,        1,           -- Bar 2(Cached), then 15/16 Tone, Ratio: 1
    ···
}
```

#### `Log {}`

```lua
--[ Example: ]--
Log {                        -- Check the "*.series" file for the output.
    Name = "Tint R",         -- tostring( os.clock ) by default
    {0}, 1, STATIC,          -- Bar 0, Val: 1, Static Ease (Ratio === 0)
    {60}, 0.5, LINEAR,       -- ...
    {61}, 1
}
```

#### `Xs {}, Ys{}, Xd{}, Cs{}`

```lua
--[ Example: ]--
Xs {                         -- Okay to omit the "Bar 0" frame.
    {30}, 1, LINEAR,         -- Xs: Scale Factor X
    {31}, 0.5, LINEAR,       -- Ys: Scale Factor Y
    {32}, 1                  -- Cs: Flow Speed Factor, Designer Side
}                            -- Xd: Offset X by Px
```

#### `Wish {}`

```lua
--[ Example: ]--
local myWish = Wish {        -- When failed, a nil will be returned.
    WithDt = true,           -- true by default
    Special = true,          -- false by default
    {1}, 4, 3, LINEAR,       -- Bar 1, X=4, Y=3, Linear Ease

    -- Add Radius(5 here) & Degree(0 here) like this
    -12, oldWish {1,-12}.x, oldWish {1,-12}.y, {5, 0, LINEAR},
    ···
}
```

#### `Helper {}`

```lua
--[ Example: ]--
local helper = Helper {      -- When failed, a nil will be returned.
    {1}, 4, 3, LINEAR,       -- Bar 1, X=4, Y=3, Linear Ease
    -12, 8, 9, LINEAR,       -- Bar 1 then 12/16 Tone, X=8, Y=9, Linear Ease
    ···
}   -- Then you can use the Helper to do some interpolations.
```

#### `Child {}`

```lua
--[ Example: ]--
Child {
    Wish = nil,              -- The last Wish of the Fumen by default
    Radius = 7.0,            -- 7.0 by Default
    Special = false,         -- Try to generate a special Hint if true, false by default
    InitLoop = 0.25,         -- 0.25 by default
    DeltaLoop = 1.25,        -- 0 by default
    {1, -1}, 2, 3, 4, ···    -- Times
}
```

#### `Hint {}`

```lua
--[ Example: ]--
Hint {
    Wish = myWish,           -- The last Wish of the Fumen by default
    Special = false,         -- False by default
    {1}, 1, 2, 3, 4, ···     -- Times
}
```

#### `Echo {}`

```lua
--[ Example: ]--
Echo {
    Radius = 7.0,            -- 0 by Default
    Special = false,         -- Scored if true. false by default
    InitLoop = 0.25,         -- 0.25 by default, ignored if Radius is 0
    DeltaLoop = 1.25,        -- 0 by default, ignored if Radius is 0
    {1}, 8, 0.5,             -- T1, X1, Y1
    -12, 8, 0.5,             -- T2, X2, Y2
    ···
}
```

#### Utils

```lua
--[ Usage: ]--
local cos, sin = CosSin(deg)
local since_tone_or_nil = SinceTone(set_to_or_nil)
local tone_time = Toneof(context_time)
```

## Addons

When the user types a new absolute time (`{bar, tone}`, like`{1, 1/16}`) into an API function, a **time context switching** will happen. For `Addon` authors, pay attention to:

1. Convert all user-input times into tones with `Toneof(t)` **AT FIRST**, and then store the current time context with `local S = SinceTone()` **IMMEDIATELY**;

2. **ALWAYS** call API functions with zero-bar absolute time(s) like `{0, tone}`, or `tone` time(s) after setting `SinceTone` to 0;

3. **ALWAYS** reset the time context with `SinceTone(S)` before exiting an Addon function.

Official addons are as follows, **under the `Apache-2.0` License**. Considering the code style, we decided not to let them builtin:

```lua
-- Duo
--
local DUO_RADIUS = 3
local DUO_WITHDT = false
local DUO_BEFORE_TONE = 0.5
local DUO_HINT_SPECIAL = false

local function Duo(t, x, y, ...)
    t = Toneof(t)
    local S, t0, degs = SinceTone(), SinceTone(0) or (t - DUO_BEFORE_TONE), {...}
    if t0 > 0 and degs[1] then
        Wish { Special = true, WithDt = DUO_WITHDT, t0, x, y, 0, t, x, y }
        for i = 1, #degs do
            local D = degs[i]
            if type(D) == "number" then
                Child { Radius = DUO_RADIUS, Special = DUO_HINT_SPECIAL,
                        InitLoop = D/360, t }
            else
                Child { Radius = DUO_RADIUS, Special = DUO_HINT_SPECIAL,
                        InitLoop = D[1]/360, DeltaLoop = D[2], t }
            end
        end
    end SinceTone(S)
end


-- Rail
--
local RAIL_Y = 0.5
local RAIL_RADIUS = 7
local RAIL_INITLOOP = 0.25
local RAIL_DELTALOOP = 0
local RAIL_BEFORE_TONE = 0.75
local RAIL_CHILD_SPECIAL = false
local RAIL_WISH_SPECIAL = false
local RAIL_WISH_WITHDT = true

local function Rail(x, ...)
    local args = {...}
    local arglen = #args
    if arglen > 0 then
        for i = 1, arglen do  args[i] = Toneof( args[i] )  end
        table.sort(args)

        local tprv = args[1]
        if tprv > RAIL_BEFORE_TONE then
            local tgl, tg = 1, {tprv}
            local gsl, gs = 1, {tg}
            for i = 2, arglen do
                local tcur = args[i]
                if tcur-tprv <= RAIL_BEFORE_TONE then   tgl = tgl+1   tg[tgl] = tcur
                else                                    gsl = gsl+1   tg,tgl = {tcur},1   gs[gsl] = tg
                end                                     tprv = tcur
            end

            local S = SinceTone() ; SinceTone(0)
            for i = 1, gsl do
                local tsi = gs[i] ; tsi.Radius, tsi.Special = RAIL_RADIUS, RAIL_CHILD_SPECIAL
                                    tsi.InitLoop, tsi.DeltaLoop = RAIL_INITLOOP, RAIL_DELTALOOP
                gs[i] = Wish {  Special = RAIL_WISH_SPECIAL, WithDt = RAIL_WISH_WITHDT,
                                tsi[1]-RAIL_BEFORE_TONE, x, RAIL_Y, STATIC,
                                tsi[#tsi], x, RAIL_Y  }
                Child(tsi) end
            return SinceTone(S) or gs
        end
    end
end


-- Slide
--
local SLIDE_MOD = 4
local SLIDE_INTERVAL = 1/32

local function Slide(T)
    local tlen, end_time = #T, 0
    if tlen < 5 then
        return end
    for i = 1, tlen, 4 do
        end_time = Toneof( T[i] )
        T[i] = end_time
    end

    local S, helper = SinceTone(), Helper(T), SinceTone(0)
    local echo1 = { Radius = T.Radius, InitLoop = T.InitLoop, DeltaLoop = T.DeltaLoop                 }
    local echo2 = { Radius = T.Radius, InitLoop = T.InitLoop, DeltaLoop = T.DeltaLoop, Special = true }
    local k, e1, e2, cur = 1, 1, 1, T[1]+SLIDE_INTERVAL
    while cur < end_time do
        local xy = helper(cur)
        if k % SLIDE_MOD ~= 0 then echo1[e1], echo1[e1+1], echo1[e1+2], e1 = cur, xy.x, xy.y, e1+3
        else                       echo2[e2], echo2[e2+1], echo2[e2+2], e2 = cur, xy.x, xy.y, e2+3 end
        k, cur = k+1, cur+SLIDE_INTERVAL
    end
    Echo(echo1) ; Echo(echo2)
    SinceTone(S)
end


-- Arc
--
local function Arc(T)
    local tlen = #T
    if tlen < 9 then
        return end
    for i = 1, tlen, 5 do
        T[i] = Toneof( T[i] )
    end

    local S, W = SinceTone(), {}, SinceTone(0)
    for i = 1, (tlen-1)/5 do
        local ti, wi = i*5-4, i*8-7
        local r, d0, d1 = 0, T[ti+3], T[ti+8]
        local px, py, c0, s0 = T[ti+1], T[ti+2], CosSin( d0 )
        local dx, dy, c1, s1 = T[ti+6]-px, T[ti+7]-py, CosSin( d1 )

        if dx ~= 0 and c1 ~= c0 then        r = dx / (c1 - c0)
        elseif dy ~= 0 and s1 ~= s0 then    r = dy / (s1 - s0)
        else                                r, d0, d1 = 0, 0, 0
        end                                 px, py = (px - r*c0), (py - r*s0)

        W[wi], W[wi+1], W[wi+2], W[wi+3] = T[ti], px, py, {r, d0, T[ti+4]}
        W[wi+4], W[wi+5], W[wi+6], W[wi+7] = T[ti+5], px, py, {r, d1, 0}
    end SinceTone(S)
        W.WithDt, W.Special = T.WithDt, T.Special
    return T.Helper and Helper(W) or Wish(W)
end
```

## Example `Chart[Fumen]` Code

Author: `Clock Futuros :: The World She Messed υρ`, under the `Apache-2.0` License.

```lua
Time {
    Offset = 10,
    Tempo = { 0, 6, 4,
              1, 4, 4 },
    0, 0, 180
}
Delta {
    {0}, 1,
    {24.251}, 1011,
    {24.255}, 0.6,
    {24.5}, 0.7,
    {24.75}, 0.8,
    {24.875}, 0.9,
    {25}, 1,
    {31.001}, 4.37,
    {31.005}, 1,
    {36.252}, 1011,
    {36.256}, 0.25,
    {36,-5}, 5/16,
    -6, 6/16,
    -7, 7/16,
    -8, 8/16,
    -9, 9/16,
    -10, 10/16,
    -11, 11/16,
    -12, 12/16,
    -13, 13/16,
    -14, 14/16,
    -15, 15/16,
    {37}, 1,
    {43.001}, 6,
    {43.005}, 1,
    {48.251}, 30,
    {48.255}, 0,
    {48.376}, 30,
    {48.38}, 0,
    {48.501}, 30,
    {48.505}, 1/9,
    {48,-9}, 2/9,
    -10, 3/9,
    -11, 4/9,
    -12, 5/9,
    -13, 6/9,
    -14, 7/9,
    -15, 8/9,
    {49.001}, 3,
    {49.005}, 1,
    {55.001}, 3,
    {55.005}, 1,
    {60.25}, 0.5,
    {60.3}, 0.6,
    {60.35}, 0.7,
    {60.4}, 0.8,
    {60.45}, 0.9,
    {60.5}, 1,
    {61.751}, 20,
    {61.755}, 0.25,
    {61.876}, 20,
    {61.88}, 0.625,
    {62.001}, 20,
    {62.005}, 1/9,
    {62,-1}, 2/9,
    -2, 3/9,
    -3, 4/9,
    -4, 5/9,
    -5, 6/9,
    -6, 7/9,
    -7, 8/9,
    {62.501}, 3.7,
    {62.505}, 1,
    {64.001}, 6,
    {64.005}, 1,
    {64.626}, 6,
    {64.63}, 1,
    {64.751}, 3.7,
    {64.755}, 1,
    {65.126}, 3.7,
    {65.13}, 1,
    {65.501}, 1011,
    {65.505}, 4/16,
    {65,-9}, 5/16,
    -10, 6/16,
    -11, 7/16,
    -12, 8/16,
    -13, 9/16,
    -14, 10/16,
    -15, 11/16,
    -16, 12/16,
    -17, 13/16,
    -18, 14/16,
    -19, 15/16,
    {66.251}, 0.73,
    {66.75, 1/64}, -0.85,
    2/64, 1.7,
    3/64, -0.8,
    4/64, 1.6,
    5/64, -0.75,
    6/64, 1.5,
    7/64, -0.7,
    8/64, 1.4,
    9/64, -0.65,
    10/64, 1.3,
    11/64, -0.6,
    12/64, 1.2,
    13/64, -0.55,
    14/64, 1.1,
    15/64, -0.5,
    16/64, 1,
    {71.751}, 3.7,
    {71.755}, 1,
    {72.001}, 3.7,
    {72.01}, 0.88,
    {73}, 0.5,
    -13, 13/24,
    -14, 14/24,
    -15, 15/24,
    -16, 16/24,
    -17, 17/24,
    -18, 18/24,
    -19, 19/24,
    -20, 20/24,
    -21, 21/24,
    -22, 22/24,
    -23, 23/24,
    -24, 1,
    {74.626}, 0.88,
    {74.751}, 6,
    {74.755}, 0.88,
    {74.875}, 1,
    {75.001}, 0.87,
    {75.126}, 6,
    {75.13}, 0.87,
    {75.25}, 1,
    {75.376}, 0.86,
    {75.501}, 6,
    {75.505}, 0.86,
    {75.625}, 1,
    {75.751}, 0.85,
    {75.876}, 6,
    {75.88}, 0.85,
    {76}, 1,
    {76.126}, 0.84,
    {76.251}, 6,
    {76.255}, 0.84,
    {76.375}, 1,
    {76.501}, 0.83,
    {76.626}, 6,
    {76.63}, 0.83,
    {76.75}, 1,
    {76.876}, 0.82,
    {77.001}, 6,
    {77.005}, 0.82,
    {77.125}, 1,
    {77.251}, 0.81,
    {77.376}, 6,
    {77.38}, 0.81,
    {77.5}, 1,
    {77.626}, 0.8,
    {77.751}, 6,
    {77.755}, 0.8,
    {77.875}, 1,
    {78.001}, 0.79,
    {78.126}, 6,
    {78.13}, 0.79,
    {78.25}, 1,
    {78.376}, 0.78,
    {78.501}, 6,
    {78.505}, 0.78,
    {78.625}, 1,
    {78.751}, 0.77,
    {78.876}, 6,
    {78.88}, 0.77,
    {79}, 1,
    {79.126}, 0.76,
    {79.251}, 6,
    {79.255}, 0.76,
    {79.375}, 1,
    {79.501}, 0.75,
    {79.626}, 6,
    {79.63}, 0.75,
    {79.75}, 1,
    {79.876}, 0.74,
    {80.001}, 6,
    {80.005}, 0.74,
    {80.125}, 1,
    {80.251}, 0.73
}

-- Since {0}
--
Wish {
    {0}, 8, 4, INSINE,
    {1}, 8, 0.5
}
Wish {
    Special = true,
    {0, 1}, 8, 0.5, STATIC,
    {1}, 8, 0.5
} ; Hint{0}
Wish {
    {3.5}, 8, 7.5, INSINE,
    {4}, 8, 0.5
} ; Hint{0}

Wish {   -- A1
    {0.5}, 8, 7.5, INSINE,
    {1}, 8, 0.5, OUTSINE,
    {2.5}, 5.5, 0.5, INSINE,
    {4}, 8, 0.5, OUTSINE,
    {6.25}, 10.5, 0.5
}
Child{ {1.75}, {2.875}, {5}, {6.25} }

Wish {   -- A2
    {1}, 8, 0.5, OUTSINE,
    {2.5}, 7.25, 0.5, INSINE,
    {4}, 8, 0.5, OUTSINE,
    {5.25}, 8.5, 0.5
}
Child{ {2.25}, {5.25} }

Wish {   -- A3
    {1}, 8, 0.5, OUTSINE,
    {2.5}, 8.75, 0.5, INSINE,
    {4}, 8, 0.5, OUTSINE,
    {5.25}, 7.5, 0.5
}
Child{ {2.5} }

Wish {   -- A4
    {1}, 8, 0.5, OUTSINE,
    {2.5}, 10.5, 0.5, INSINE,
    {4}, 8, 0.5, OUTSINE,
    {6.25}, 5.5, 0.5
}
Child{ {2}, {3.25}, {4.75}, {5.5} }
Child{ InitLoop = 17/64, {5.5} }
Rail(8, {5.875})

-- Since {7}
--
Wish {   -- B1
    {7}, 6.5, 0.5, OUTSINE,
    {8}, 5.5, 0.5, INSINE,
    {9}, 6.5, 0.5, OUTSINE,
    {10}, 7.5, 0.5, INSINE,
    {11.5}, 6, 0.5, STATIC,
    {11.5}, 6, 2.5, {2, -90, LINEAR},
    {12.25}, 6, 2.5, {2, 0, STATIC},
    {12.25}, 8, 2.5, OUTSINE,
    {13}, 8, 9
}
Wish {   -- B2
    {7}, 9.5, 0.5, OUTSINE,
    {8}, 10.5, 0.5, INSINE,
    {9}, 9.5, 0.5, OUTSINE,
    {10}, 8.5, 0.5, INSINE,
    {11.5}, 10, 0.5, STATIC,
    {11.5}, 10, 2.5, {2, 270, LINEAR},
    {12.25}, 10, 2.5, {2, 180, STATIC},
    {12.25}, 8, 2.5, OUTSINE,
    {13}, 8, 9
}
Wish {
    Special = true,
    {11.5}, 8, 2.5, STATIC,
    {12.25}, 8, 2.5
} ; Hint{0}

Wish {   -- B3
    {7}, 8, 0.5, STATIC,
    {11.25}, 8, 0.5
}
Child {
    {10.5}, {11.25}
}

Duo({7}, 8,4, 0,90,180,270)
Duo({11.5}, 8,5.5, 45,135,225,315)

DUO_RADIUS = 2
Duo({7.75}, 5,4, 60,240)
Duo({8}, 11,4, 60,240)
Duo(-4, 7,4, 60,240)
Duo(-8, 9,4, 60,240)
Duo(-14, 5,4, 60,240)
Duo({9,-4}, 11,4, 60,240)
Duo(-8, 9,4, 60,240)
Duo(-12, 7,4, 60,240)
Duo({10}, 10,4, 60,240)
Duo(-12, 6,4, 60,240)

-- Since {12.75}
--
Rail(4,
    {25}, {35.25}, {35.5}
)
Rail(5,
    {12.75}, {13.25}, {14.25}, {17.5},
    {20.25}, {25.75}, {28.75},
    {31.25}, {31.75}, {35.875}
)

Rail(6,
    {14.875}, {16.5}, {18.25},
    {21.25}, {23.25}, {24}, {24.75}, {25.25}, {27.25}, {28},
    {30.25}, {33.75}, {34.75}, {35.375}, {36.25}
)
Rail(6.5,
    {22.625}, {31}
)

Rail(7,
    {12.875}, {13.125}, {13.75}, {17}, {19.75},
    {20.5}, {25.75}, {27.5},
    {31.75}, {33}, {34}, {35}
)
Rail(7.875,
    {27.75}
)

Rail(8,
    {14.5}, {15.5}, {16.25}, {17.875}, {18.125}, {18.75}, {19.25},
    {21.5}, {23}, {24.875}, -4, {26.875}, {27.25}, {28.5}, {29}, {29.875},
    {30.125}, {30.75}, -2, -6, {31.5}, {32}, {33.875}, {34.5},
    {35.25}, {35.5}, {35.875}, -4
)
Rail(8.125,
    {27.75}
)
Rail(8.75,
    {18}
)
Rail(8.875,
    {30}, {32.25}
)

Rail(9,
    {13}, {13.5}, {15.25}, {15.75}, {16.75}, {19.5},
    {25.5}, {30.375}, {32.5}, -4, {33.625}, {36}
)
Rail(9.125,
    {32.25}
)
Rail(9.5,
    {22.75}, {31}
)

Rail(10,
    {14}, {15.875}, {18.875}, -4,
    {20}, {20.875}, {21.75}, {25}, {27.125}, {27.875}, {28.25},
    {34}, {36.25}
)
Rail(10.5,
    {15.125}
)

Rail(11,
    {13.375}, {16}, {17.25}, {19.375},
    {25.375}, {26}, {28.75},
    {30.25}, {31.375}, {32.5}, {33.375}
)
Rail(12,
    {19}, {22}, {28}
)

DUO_RADIUS = 1.75
Duo({22.375}, 12,5, {60,-1/8},{240,-1/8})
Duo({23.5}, 4,5, {120,1/8},{300,1/8})
Duo({24.25}, 8,5, 90,225,315)
Duo({26.25}, 6,5, 120,300)
Duo({26.5}, 10,5, 120,300)
Duo({29.25}, 10,5, 60,240)
Duo({29.5}, 6,5, 60,240)
Duo({33.25}, 5,3.5, 30,210)
Duo({34.75}, 11,3.5, 150,330)

-- Since {37}
--
Rail(4,
    {37.5}, {39.375},
    {40.375}, {43.375}, {45.25}, {48.125},
    {50.5}, {51.25}, {56.75}, {59.25},
    {60.75}
) ; Child{ InitLoop = 17/64, {60.75} }
Rail(4.5,
    {44.75}, {59,-15}
)

Rail(5,
    {37}, {38},
    {40}, {43}, {45.625}, {46.25}, {49.5},
    {52.375}, -2, -4, -11, -13, {54.5}, {55}, {58.875}
)
Rail(5.5,
    {40.625}, {43.625}, {48},
    {56}, -1, -7,
    {60,-1}
)
Rail(5.875,
    {45.75}, {57.75}
)

Rail(6,
    {39.25}, {39.75},
    {40.25}, {42.25}, {43.25}, {44.5}, {45.125}, {47.5}, {49.375}, {49.625},
    {50.125}, {50.625}, {51.125}, {51.625}, {53,-7}, {54.25},
    {56.875}, {57.875}, {58}, {58.5}, {59}, {60.875}
) ; Child{ InitLoop = 17/64, {60.875} }
Rail(6.125,
    {45.75}, {57.75}
)
Rail(6.5,
    {56,-5}, {60,-3}
)
Rail(6.875,
    {59.5}
)

Rail(7,
    {38.5}, {39.625},
    {41}, {41.75}, {42.125}, {42.75}, {44.25}, {44.875}, 1,
    {46.25}, {47}, {47.875}, {49},
    {50.25}, 1/12, {51,-15}, {52,-13}, -15, {53,-5}, {54},
    {55}, {55.25}, {58.25}, {58.625}, {59.125}
)
Rail(7.125,
    {59.5}
)
Rail(7.5,
    {56,-3}
)
Rail(7.875,
    {50.75}
)

RAIL_INITLOOP = 15/64
Rail(8, {39.5}) ; Child{ InitLoop = 17/64, 0 }
Rail(8, {46}) ; Child{ InitLoop = 17/64, 0 }

RAIL_INITLOOP = 0.25
Rail(8,
    {40}, {41.25}, {41.5}, {42.625},
    {45.25}, {46.5}, {47.25}, {47,-11}, {48,-1}
) ; Child{ InitLoop = 17/64, {47.25} }

Rail(8,
    {49.25}, {49.75},
    {51}, {51.5}, {52,-1}, -3, {52.25,-3}, -5, {53.625}, {53.875}, {54.25},
    {55.375}, {56.75}, {57}, {57,-7}, -9, -13, -15
)
Rail(8.125,
    {50.75}
)
Rail(8.5,
    {60}
)
Rail(8.875,
    {44.25}, {52.75}, {53.25}
)

Rail(9,
    {38},
    {41}, 0.75, 1.125, 1.75, {45.125}, -4, {46.625}, {47.125}, {47.875},
    {50, 7/24}, 9/24, {51.875}, {52.875}, {54.375},
    {55.25}, {55.75}, {56.375}, {58.125}, -8, {59.125}, -5
)
Rail(9.125,
    {44.25}, {52.75}, {53.25}
)
Rail(9.5,
    {38.5}, {60.125}
)
Rail(9.875,
    {56.25}
)

Rail(10,
    {37.375}, {38.875}, -6, {39.625}, -4,
    {40.75}, {41.875}, {43.5}, {44.375}, {45,-13}, {46.75}, {47.5}
) ; Child{ InitLoop = 15/64, {46.75} }
Rail(10,
    {49.125}, -8, -16, -24, -32, -40,
    {53.375}, -8, {55.625}, {56.625}, {57.375}, -2, -4, {58.25}, {59.625},
    {60,-15}
) ; Child{ InitLoop = 15/64, {60,-15} }
Rail(10.125,
    {56.25}
)
Rail(10.375,
    {60.25}
)
Rail(10.5,
    {43.25}, {47.75}, {57.125}, {57,-3}
)
Rail(10.625,
    {60.25}
)
Rail(10.875,
    {52.25}
)

Rail(11,
    {37},
    {40.625}, {43}, {44.5}, {45}, {45.5}, {45,-15}, {46.625},
    {52}, {52.125}, {53}, {53.125}, {54.125}, {54.625}, {55.75}, {56.125}, {58.375}
)
Rail(11.125,
    {52.25}
)
Rail(11.5,
    {37.625}, {38.75}, -6, {39.75},
    {44.75}, {59.875}
)

Rail(12,
    {37.25}, {38.25},
    {40.375}, {43.75}, {47.625}, {49}, {49.25}, {49.75},
    {50.5}, {51.25}, {56.5}, {59.25},
    {60,-13}
) ; Child{ InitLoop = 15/64, {60,-13} }

DUO_RADIUS = 1
Duo({60,-8}, 7.125,4, 135,225)
Duo({60,-9}, 8.875,4, 45,315)

DUO_RADIUS = 1.75
Duo({41.25}, 4.5,4, 135,225)
Duo({41.5}, 11.5,4, 45,315)

DUO_RADIUS = 2
Duo({60,-10}, 7.125,4, 150,210)
Duo({60,-11}, 8.875,4, 30,330)

DUO_RADIUS = 2.5
Duo({53.5}, 11,4, 30, 330)
Duo({53.75}, 5,4, 150, 210)
Duo({57.25}, 6,4, 155, 205)
Duo({58.75}, 10,4, 30, 330)

DUO_RADIUS = 2.75
Duo({44}, 8,2, 25, 155)
Duo({48.25}, 8,4.75, 45,135,225,315)
Duo({48.375}, 8,4.75, 30,150,210,330)
Duo({48.5}, 8,4.75, 15,165,195,345)
Duo({51.75}, 8,2.5, 80, 100)
Duo({54.75}, 8,4, 130, 50)
Duo({59.75}, 8,2, 83, 97)

DUO_RADIUS = 4
Duo({61}, 8,5.75, 170, 10)

-- Since {61}
--
Wish {
    {61}, 8, 4, OUTSINE,
    {61.75}, 5, 4, STATIC,
    {62}, 5, 4
}
Child { Radius = 2.75, InitLoop = 0.125,
        {61.75}, -2, -4 }
Child { Radius = 2.75, InitLoop = 0.5,
        {61.75}, -2, -4 }
Child { Radius = 2.75, InitLoop = 0.875,
        {61.75}, -2, -4 }
Wish {
    {61}, 8, 4, OUTSINE,
    {61.75}, 11, 4, STATIC,
    {62}, 11, 4
}
Child { Radius = 2.75, InitLoop = 0,
        {61.75}, -2, -4 }
Child { Radius = 2.75, InitLoop = 0.375,
        {61.75}, -2, -4 }
Child { Radius = 2.75, InitLoop = 0.625,
        {61.75}, -2, -4 }
Wish {
    {61}, 8, 4, INSINE,
    {61.625}, 8, 9
}
Wish {
    {61}, 8, 4, INSINE,
    {61.625}, 8, -1
}

Rail(4,
    {62.5},
    {75.625}, -2, {76.375}, -2, {79.875}
)
Rail(4.5,
    {74.5}, -2
)

Rail(5,
    {68.625}, -2, {69}, -2,
    {71.75}, {79.125}, {79.75}
)
Rail(5.25,
    {63.125}
)

Rail(6,
    {62.75}, {63.75,-1}, {64.25}, {64.875}, {65.375}, {66.25}, {70.75}
) ; Child{ InitLoop = 17/64, 0 }
Rail(6,
    {71.75,-1}, {75.625}, -2, {76.375}, -2, {79}
)
Rail(6.5,
    {71.625}
)

Rail(7,
    {62.625}, {63.125}, {63.5}, {64.5},
    {67.375}, {67.75}, -2, {68.5}, -3, {68.875}, -3, {69.25}, -2, {69.875},
    {70.25}, {71}, 2/24, -5, -7, {75.25}, -2, {76.75}, -2, {79.75}
)
Rail(7.5,
    {74.5}, -2
)

RAIL_INITLOOP = 15/64
Rail(8, {62.5}) ; Child{ InitLoop = 17/64, 0 }
Rail(8, {67}) ; Child{ InitLoop = 17/64, 0 }
RAIL_INITLOOP = 0.25

Rail(8,
    {63.25}, {63.625}, {64}, {64.375}, {64.75}, {65.125},
    {70}, {70.375}, -4, -8, {79}, {79.5}, {79.875}, -4
)
Rail(8.5,
    {76}, -2
)
Rail(8.875,
    {63}, {63.75}
)

Rail(9,
    {64.125}, {67,-3}, -6, -9, -12, -14, {68,-6}, -8, -14, {69.25}, -2,
    {70.125}, {70.5}, {71,1/24}, 3/24, -6,
    {75.25}, -2, {76.75}, -2, {77.125}, -2, {79.125}, -4
)
Rail(9.125,
    {63.75}
)
Rail(9.5,
    {71.5,-3}
)
Rail(9.75,
    {63.25}
)

Rail(10,
    {62.75}, {63.875}, {64.625}, {65}, {65.25}, {66.25},
    {70.25}, {70.75}, {72,-1}, {74.875}, -2,
    {80.125}
)
Rail(11,
    {67.125}, -2, {67.5}, -2, {70}
) ; Child{ InitLoop = 15/64, {70} }

Rail(11,
    {72}, {77.125}, -2, {79.375}
)
Rail(11.5,
    {76}, -2
)
Rail(12,
    {63.625}, {74.875}, -2, {79.5}
)

RAIL_Y = 4.25
RAIL_RADIUS = 3.5
RAIL_BEFORE_TONE = 0.75

Rail(5, {77.5}, -2)
Child{ Radius = 3.5, InitLoop = 0.75, 0, -2 }
Rail(7, {77.5}, -2)
Child{ Radius = 3.5, InitLoop = 0.75, 0, -2 }

Rail(9, {77.875}, -2)
Child{ Radius = 3.5, InitLoop = 0.75, 0, -2 }
Rail(11, {77.875}, -2)
Child{ Radius = 3.5, InitLoop = 0.75, 0, -2 }

Rail(4, {78.25}, -2)
Child{ Radius = 3.5, InitLoop = 0.75, 0, -2 }
Rail(6, {78.25}, -2)
Child{ Radius = 3.5, InitLoop = 0.75, 0, -2 }

Rail(10, {78.625}, -2)
Child{ Radius = 3.5, InitLoop = 0.75, 0, -2 }
Rail(12, {78.625}, -2)
Child{ Radius = 3.5, InitLoop = 0.75, 0, -2 }

DUO_RADIUS = 1.5
Duo({79.25}, 7,4, 90,270)
Duo({79.625}, 10,4, 90,270)
Duo({80}, 6,4, 90,270)
Duo({80.25}, 9,2.25, 90,270)

DUO_RADIUS = 2
Duo({64}, 4,4.5, 135,225)
Duo({64.375}, 12,4.5, 45,315)
Duo({64.75}, 4,2.5, 135,225)
Duo({65.125}, 12,2.5, 45,315)
Duo({66.625}, 12,3.25, 30,330)
Duo({66.875}, 4,3.25, 150,210)
Duo({68}, 5,4, 135,225)
Duo({68}, 11,4, 45,315)
Duo({69.5}, 4.25,3.5, 135,225)
Duo({69.5}, 11.75,3.5, 45,315)
Duo({72.25}, 8,4, 70,110,250,290)

DUO_RADIUS = 3
Duo({65.5}, 8,4, 70,110)
Duo({70.5}, 4.5,3.75, 135,225)
Duo({71.25}, 11.5,3.75, 45,315)

DUO_RADIUS = 3.5
Duo({80.5}, 8,5, 0,60,120,180)

DUO_RADIUS = 4.25
Duo({68.125}, 8,2.25, 85,95)
Duo({69.625}, 8,2.25, 83,97)

DUO_RADIUS = 5
DUO_WITHDT = true
Duo({71.5}, 8,1.125, 90) ; Child{ Radius = 5.5, {71.5} }
Duo({73}, 8,4, 0,180)

Wish {
    Special = true,
    {65.75}, 8, 4, STATIC,
    {66.25}, 8, 4
}
Wish {
    {65.5}, 8, 4, {3, 270, INSINE},
    {65.875}, 8, 4, {1.5, 270, INSINE},
    {66.25}, 8, 4, {0, 90}
}
Wish {
    {65.5}, 8, 4, {2, -90, OUTSINE},
    {65.875}, 8, 4, {0.75, -90, INSINE},
    {66.25}, 8, 4, {0, 90}
}
Wish {
    {65.5}, 8, 4, {3, 90, INSINE},
    {65.875}, 8, 4, {1.5, 90, INSINE},
    {66.25}, 8, 4, {0, -90}
}
Wish {
    {65.5}, 8, 4, {2, 90, OUTSINE},
    {65.875}, 8, 4, {0.75, 90, INSINE},
    {66.25}, 8, 4, {0, 270}
} ; Hint{0}

Wish {   -- C1
    {72.75}, 6.5, 0.5, STATIC,
    {73}, 6.5, 3.5, {3, 270, INSINE},
    {73.75}, 6.5, 3.5, {2.5, 180}
}
Wish {
    {73.25}, 4, 3.5, STATIC,
    {73.75}, 4, 3.5
} ; Hint{0}

Wish {   -- C2
    {72.875}, 8, 0.5, STATIC,
    {73}, 8, 0.5, INSINE,
    {74}, 8, 4.5
}
Wish {
    {73.5}, 8, 4.5, STATIC,
    {74}, 8, 4.5
} ; Hint{0}

Wish {   -- C3
    {73}, 9.5, 5.5, {5, -90, INSINE},
    {74.25}, 9.5, 5.5, {2.5, 0}
}
Wish{
    {73.75}, 12, 5.5, STATIC,
    {74.25}, 12, 5.5
} ; Hint{0}

-- Since {81}
--
Wish {
    {80.5}, 8, 0.5, OUTSINE,
    {82}, 4, 0.5, INSINE,
    {83.5}, 8, 0.5
}
Child {
    {81.125}, -4, -8, -12, -16, -20, -24, -28, -32, -36
}

Wish {
    {80.5}, 8, 0.5, OUTSINE,
    {82}, 12, 0.5, INSINE,
    {83.5}, 8, 0.5
}
Child {
    {81}, -4, -8, -12, -16, -20, -24, -28, -32, -36
}
```
