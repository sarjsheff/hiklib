# hiklib
Go wrapper for hikvision sdk

# Example

With Device Network SDK_Linux64 downloaded from https://www.hikvision.com/en/support/tools/hitools/clf4633a00e385d6ea/

```
mkdir htest  
cd htest 
go mod init example.com/htest
```

Download Device Network SDK_Linux64 (EN-HCNetSDKV6.1.9.48_build20230410_linux64.zip) and unzip.

Create file main.go

```go
package main

import (
        "log"
        "github.com/sarjsheff/hiklib"
)

func main() {
        log.Println(hiklib.HikVersion())
}
```

and run it

```shell
docker run --platform linux/amd64 -it --rm  -v $(pwd)/EN-HCNetSDKV6.1.9.48_build20230410_linux64:/hiksdk -v $(pwd):/app -w /app -e LD_LIBRARY_PATH="/hiksdk/lib" -e CGO_CXXFLAGS="-I/hiksdk/incEn/" -e CGO_LDFLAGS="-L/hiksdk/lib -lhcnetsdk" golang go get github.com/sarjsheff/hiklib
docker run --platform linux/amd64 -it --rm  -v $(pwd)/EN-HCNetSDKV6.1.9.48_build20230410_linux64:/hiksdk -v $(pwd):/app -w /app -e LD_LIBRARY_PATH="/hiksdk/lib" -e CGO_CXXFLAGS="-I/hiksdk/incEn/" -e CGO_LDFLAGS="-L/hiksdk/lib -lhcnetsdk" golang go run .
```

You should see the Hikvision Device Network SDK_Linux64 version number

```shell
2024/07/27 05:44:13 HCNetSDK V6.1.9.48
```
## Get snapshot

```go
package main

import (
	"log"

	"github.com/sarjsheff/hiklib"
)

func main() {
	log.Println(hiklib.HikVersion())
	u, dev := hiklib.HikLogin("<camera ip>",8000, "<user>", "<password>")
	hiklib.HikCaptureImage(u, dev.ByStartChan, "./cam.jpeg")
}
```
