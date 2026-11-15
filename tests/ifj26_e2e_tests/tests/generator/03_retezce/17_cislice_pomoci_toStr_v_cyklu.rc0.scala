import ifj26.*

@main def main(): Unit =
  var s = ""
  var i = 1
  while i <= 5 do
    val d = toStr(i)
    s = s + d
    i = i + 1
  print(s)
  print("\n")
