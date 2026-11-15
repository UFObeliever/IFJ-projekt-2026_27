import ifj26.*

@main def main(): Unit =
  var i = 1
  while i <= 5 do
    var j = 1
    while j <= 5 do
      val p = i * j
      print(p)
      print(" ")
      j = j + 1
    print("\n")
    i = i + 1
