import ifj26.*

def dbl(x: Int): Int = x * 2

@main
def main(): Unit =
  var i = 0
  while dbl(i) < 10 do
    print(toStr(i))
    i = i + 1
