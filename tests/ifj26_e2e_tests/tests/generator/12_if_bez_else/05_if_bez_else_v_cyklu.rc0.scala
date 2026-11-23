import ifj26.*

@main def main(): Unit =
  var i = 0
  var s = 0
  while i < 10 do
    if i / 2 * 2 == i then
      s = s + i
    i = i + 1
  print(s)
  print("\n")
