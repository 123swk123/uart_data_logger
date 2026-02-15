-- transparent output format 'c' tests
-- config.txt (baud rate: 921600, 8 data bits, no parity, 1 stop bit)

-- s1:921600,8,0,1,c,13,13
-- t:2026,2,12,16,32

print("Starting the test...transparent output format 'c'")
tio.msleep(100)

test_lines = {
    "ASCII test line 1",
    "ASCII test line 2",
    "Dolor pariatur ipsum nisi dolore culpa in sunt Lorem est. Elit exercitation ut cillum minim velit consequat irure laboris voluptate aliqua ex nisi eu incididunt. Labore excepteur ut pariatur labore culpa esse cillum irure ex cillum sunt fugiat do nisi. Officia irure excepteur ad sint pariatur dolor. Tempor dolor tempor laborum.",
    "Esse non do amet pariatur esse proident. Irure labore cillum est commodo. Commodo anim aute veniam cupidatat sit aute adipisicing. Cillum cupidatat do ea do cupidatat velit ut consequat minim adipisicing consequat.",
    "Lorem labore voluptate officia non deserunt laboris ea. Nulla officia nulla sint deserunt quis ullamco sint laborum. Anim excepteur cillum dolor consequat laborum occaecat dolor minim. Aute nisi ullamco excepteur qui minim duis ex. Dolore deserunt in fugiat ex non elit quis id laboris pariatur minim. Velit aliqua ad consectetur nostrud id enim culpa commodo dolore incididunt incididunt. Cillum veniam consectetur labore consectetur quis ea quis excepteur. Do est aute reprehenderit dolor culpa ea in fugiat consectetur consectetur culpa consequat voluptate.",
    "Elit labore voluptate tempor est nostrud consequat esse enim et consectetur. Velit cupidatat aliqua commodo irure magna eiusmod ea duis ullamco eu voluptate laborum id occaecat. Aliquip deserunt proident exercitation. Sit aliquip sit elit consectetur nulla dolor consectetur dolor occaecat cupidatat proident veniam ea qui laboris. Pariatur esse dolor anim. Non laborum duis nulla laborum fugiat tempor quis cillum aliqua anim.",
    "Reprehenderit aliqua ut pariatur laboris dolor pariatur ex proident aute aliqua veniam Lorem elit ex. Id est elit qui ipsum id consequat deserunt enim eu eiusmod sit labore quis. Sunt et minim officia labore occaecat et anim commodo non ex cillum irure amet Lorem dolor. Eu pariatur culpa irure in aliqua amet non pariatur eu exercitation Lorem.",
}

for i, line in ipairs(test_lines) do
    print("Sending line " .. i)
    tio.msleep(50)
    tio.write(line .. "\r")
end
