-- config.txt (baud rate: 921600, 8 data bits, no parity, 1 stop bit)

-- s1:921600,8,0,1,u,255,255
-- t:2026,2,12,16,32

print("Starting the test...decimal output format 'u'")
tio.msleep(100)

test_lines = {16, 32, 64, 128, 254}

for i, line in ipairs(test_lines) do
    tio.msleep(1000)
    print("Sending bytes " .. line)
    for j = 0, line-1 do
        tio.write(string.char(j % 256))
    end
    tio.write(string.char(255)) -- seq start/stop marker
end
