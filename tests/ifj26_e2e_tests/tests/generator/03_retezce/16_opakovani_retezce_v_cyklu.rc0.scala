import ifj26.*

@main def main(): Unit =
  var s = ""
  var i = 0
  while i < 5 do
    s = s + "ab"
    i = i + 1
  val n = strLength(s)
  print(s)
  print("\n")
  print(n)
  print("\n")
