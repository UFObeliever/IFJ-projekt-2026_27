import ifj26.*

def dbl(x: Int): Int = x * 2

@main
def main(): Unit =
  var s = 0
  var i = 0
  while i < 5 do
    s = s + dbl(i)
    i = i + 1
  print(toStr(s))
