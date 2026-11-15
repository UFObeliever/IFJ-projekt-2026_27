import ifj26.*

@main def main(): Unit =
  var i = 0
  var total = 0
  while i < 3 do
    val t = i * 10
    var j = 0
    while j < 3 do
      val t = j + 1
      total = total + t
      j = j + 1
    total = total + t
    i = i + 1
  print(total)
  print("\n")
