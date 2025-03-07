# gateway-demo-cpp for Windows
在根目录下执行 `make`，会在 output 目录下生成 echo.exe 和 gatewy.exe 
```
cd output
./gateway
```
```
curl -v http://localhost:8090/echo               # 测试echo请求
curl -v http://localhost:8090/docker-echo        # 测试docker请求
```
测试样例：
```
# echo

Invoke-WebRequest -Uri http://localhost:8090/echo -Method GET

Invoke-WebRequest -Uri http://localhost:8090/echo/echo -Method POST -Headers @{ "Content-Type" = "application/json" } -Body '{"message": "Hello, HTTP!"}'


# docker-echo

Invoke-WebRequest -Uri http://localhost:8090/docker-echo -Method GET

Invoke-WebRequest -Uri http://localhost:8090/docker-echo/echo/uppercase -Method POST -Body "hello"

Invoke-WebRequest -Uri http://localhost:8090/docker-echo/echo/primes -Method POST -Headers @{ "Content-Type" = "text/plain; charset=utf-8" } -Body "10017221"
```