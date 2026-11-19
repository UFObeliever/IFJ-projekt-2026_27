import ifj26.*

@main
def main(): Unit =
  var k = 0
  while k < 2 do
    for i <- 1 to 3 do print(i)
    print("|")
    k = k + 1
