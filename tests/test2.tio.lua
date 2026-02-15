-- config.txt (baud rate: 921600, 8 data bits, no parity, 1 stop bit)

-- s1:921600,8,0,1,x,13,13
-- t:2026,2,12,16,32

print("Starting the test...hexadecimal output format 'x'")
tio.msleep(100)

test_lines = {16, 32, 64, 128, 256}

for i, line in ipairs(test_lines) do
    tio.msleep(50)
    print("Sending bytes " .. line)
    for j = 0, line-1 do
        tio.write(string.char(j % 256))
    end
    tio.write("\r")
end
