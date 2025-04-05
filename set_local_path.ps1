$ZeroMQ_BIN = "D:\lib\zeromq-4.3.5\bin"
$env:PATH = "${ZeroMQ_BIN};" + $env:PATH
Write-Host -ForegroundColor Green "Add ${ZeroMQ_BIN} to PATH"

$spdlog_BIN = "D:\lib\spdlog-1.15.2\bin"
$env:PATH = "${spdlog_BIN};" + $env:PATH
Write-Host -ForegroundColor Green "Add ${spdlog_BIN} to PATH"

$OpenCV_BIN = "D:\lib\opencv-3.4.16\x64\vc15\bin"
$env:PATH = "${OpenCV_BIN};" + $env:PATH
Write-Host -ForegroundColor Green "Add ${OpenCV_BIN} to PATH"
