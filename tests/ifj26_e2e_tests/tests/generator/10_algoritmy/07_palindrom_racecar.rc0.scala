import ifj26.*

@main def main(): Unit =
  val s = "racecar"
  val n = strLength(s)
  var i = 0
  var ok = 1
  while i < n do
    val j = n - 1 - i
    val a = ord(s, i)
    val b = ord(s, j)
    if a != b then
      ok = 0
    else
      ok = ok
    i = i + 1
  print(ok)
  print("\n")
