import ifj26.*

@main def main(): Unit =
  var i = 1
  var s = 0
  while i <= 10000 do
    s = s + i
    i = i + 1
  print(s)
  print("\n")
