# Logger output

This logger in [APP_CONF_RUNNING_LOG](../src/app/uart_logger/Makefile#L14) mode simply creates a log file with running number under the root directory.
Every time when you push the key to start logging, logger reads the [head](../src/card_config/head) file to find the last used log file name and creates the next one.

## SD card root folder listing
![Logger SD card](<sd_card_root_listing.png>)

## Log file output

### [test1 - 8bit ASCII/transparent logging](../tests/test1.tio.lua) @ 921600bps
```log
[0:5:11.0]ASCII test line 1
[0:5:15.100]ASCII test line 2
[0:5:15.200]Dolor pariatur ipsum nisi dolore culpa in sunt Lorem est. Elit exercitation ut cillum minim velit consequat irure laboris voluptate aliqua ex nisi eu incididunt. Labore excepteur ut pariatur labore culpa esse cillum irure ex cillum sunt fugiat do nisi. Officia irure excepteur ad sint pariatur dolor. Tempor dolor tempor laborum.
[0:5:15.300]Esse non do amet pariatur esse proident. Irure labore cillum est commodo. C consequat.
[0:5:15.300]Lorem labore voluptate officia non deserunt laboris ea. Nulla officia nulla sint deserunt quis ullamco sint laborum. Anim excepteur cillum dolor consequat laborum occaecat dolor minim. Aute nisi ullamco excepteur qui minim duis ex. Dolore deserunt in fugiat ex non elit quis id laboris pariatur minim. Velit aliqua ad consectetur nostrud id enim culpa commodo dolore incididunt incididunt. Cillum veniam consectetur labore consectetur quis ea quis excepteur. Do est aute reprehenderit dolor culpa ea in fugiat consectetur consectetur culpa consequat voluptate.
[0:5:15.400]Elit labore voluptate tempor est nostrud consequat esse enim et consectetur. Velit cupidatat aliqua commodo irure magna eiusmod ea duis ullamco eu voluptate laborum id occaecat. Aliquip deserunt proident exercitation. Sit aliquip sit elit consectetur nulla dolor consectetur dolor occaecat cupidatat proident veniam ea qui laboris. Pariatur esse dolor anim. Non laborum duis nulla laborum fugiat tempor quis cillum aliqua anim.
[0:5:15.500]Reprehenderit aliqua ut pariatur laboris dolor pariatur ex proident aute aliqua veniam Lorem elit ex. Id est elit qui ipsum id consequat deserunt enim eu eiusmod sit labore quis. Sunt et minim officia labore occaecat et anim commodo non ex cillum irure amet Lorem dolor. Eu pariatur culpa irure in aliqua amet non pariatur eu exercitation Lorem.
```

### [test2 - hexadecimal encoded logging](../tests/test2.tio.lua) @ 921600bps
```log
[0:23:15.800]1 2 3 4 5 6 7 8 9 a b c 
[0:23:26.800]e f 
[0:23:26.800]1 2 3 4 5 6 7 8 9 a b c 
[0:23:26.900]e f 10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f 
[0:23:26.900]1 2 3 4 5 6 7 8 9 a b c 
[0:23:27.0]e f 10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f 20 21 22 23 24 25 26 27 28 29 2a 2b 2c 2d 2e 2f 30 31 32 33 34 35 36 37 38 39 3a 3b 3c 3d 3e 3f 
[0:23:27.0]1 2 3 4 5 6 7 8 9 a b c 
[0:23:27.0]e f 10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f 20 21 22 23 24 25 26 27 28 29 2a 2b 2c 2d 2e 2f 30 31 32 33 34 35 36 37 38 39 3a 3b 3c 3d 3e 3f 40 41 42 43 44 45 46 47 48 49 4a 4b 4c 4d 4e 4f 50 51 52 53 54 55 56 57 58 59 5a 5b 5c 5d 5e 5f 60 61 62 63 64 65 66 67 68 69 6a 6b 6c 6d 6e 6f 70 71 72 73 74 75 76 77 78 79 7a 7b 7c 7d 7e 7f 
[0:23:27.0]1 2 3 4 5 6 7 8 9 a b c 
[0:23:27.100]e f 10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f 20 21 22 23 24 25 26 27 28 29 2a 2b 2c 2d 2e 2f 30 31 32 33 34 35 36 37 38 39 3a 3b 3c 3d 3e 3f 40 41 42 43 44 45 46 47 48 49 4a 4b 4c 4d 4e 4f 50 51 52 53 54 55 56 57 58 59 5a 5b 5c 5d 5e 5f 60 61 62 63 64 65 66 67 68 69 6a 6b 6c 6d 6e 6f 70 71 72 73 74 75 76 77 78 79 7a 7b 7c 7d 7e 7f 80 81 82 83 84 85 86 87 88 89 8a 8b 8c 8d 8e 8f 90 91 92 93 94 95 96 97 98 99 9a 9b 9c 9d 9e 9f a0 a1 a2 a3 a4 a5 a6 a7 a8 a9 aa ab ac ad ae af b0 b1 b2 b3 b4 b5 b6 b7 b8 b9 ba bb bc bd be bf c0 c1 c2 c3 c4 c5 c6 c7 c8 c9 ca cb cc cd ce cf d0 d1 d2 d3 d4 d5 d6 d7 d8 d9 da db dc dd de df e0 e1 e2 e3 e4 e5 e6 e7 e8 e9 ea eb ec ed ee ef f0 f1 f2 f3 f4 f5 f6 f7 f8 f9 fa fb fc fd fe ff
```

### [test3 - decimal encoded logging](../tests/test3.tio.lua) @ 921600bps
```log
[0:0:4.200]0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 
[0:1:14.200]0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 
[0:1:15.200]0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 
[0:1:16.200]0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 91 92 93 94 95 96 97 98 99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122 123 124 125 126 127 
[0:1:17.200]0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 91 92 93 94 95 96 97 98 99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122 123 124 125 126 127 128 129 130 131 132 133 134 135 136 137 138 139 140 141 142 143 144 145 146 147 148 149 150 151 152 153 154 155 156 157 158 159 160 161 162 163 164 165 166 167 168 169 170 171 172 173 174 175 176 177 178 179 180 181 182 183 184 185 186 187 188 189 190 191 192 193 194 195 196 197 198 199 200 201 202 203 204 205 206 207 208 209 210 211 212 213 214 215 216 217 218 219 220 221 222 223 224 225 226 227 228 229 230 231 232 233 234 235 236 237 238 239 240 241 242 243 244 245 246 247 248 249 250 251 252 253
```
