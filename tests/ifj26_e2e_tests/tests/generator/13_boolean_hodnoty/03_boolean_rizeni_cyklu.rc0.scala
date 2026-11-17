import ifj26.*

@main def main(): Unit =
  var i = 0
  var go = i < 5
  while go do
    i = i + 1
    go = i < 5
  print(i)
  print("\n")
