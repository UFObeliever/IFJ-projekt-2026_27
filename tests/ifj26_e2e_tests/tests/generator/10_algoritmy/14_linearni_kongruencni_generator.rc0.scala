import ifj26.*

@main def main(): Unit =
  var x = 1
  var i = 0
  while i < 10 do
    x = (x * 5 + 3) - (x * 5 + 3) / 16 * 16
    print(x)
    print(" ")
    i = i + 1
  print("\n")
