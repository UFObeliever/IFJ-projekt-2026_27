import ifj26.*

@main def main(): Unit =
  var i = 0
  while i < 5 do
    val sq = i * i
    var acc = sq + 1
    acc = acc * 2
    print(acc)
    print(" ")
    i = i + 1
  print("\n")
