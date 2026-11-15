import ifj26.*

@main def main(): Unit =
  var i = 1
  while i <= 4 do
    var j = 0
    while j < i do
      print("*")
      j = j + 1
    print("\n")
    i = i + 1
