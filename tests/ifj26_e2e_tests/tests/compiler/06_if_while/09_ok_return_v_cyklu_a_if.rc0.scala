import ifj26.*

def find(n: Int): Int =
  var i = 0
  while i < 100 do
    if i * i > n then
      return i
    else
      i = i + 1
  return -1

@main def main(): Unit =
  val m = 0
