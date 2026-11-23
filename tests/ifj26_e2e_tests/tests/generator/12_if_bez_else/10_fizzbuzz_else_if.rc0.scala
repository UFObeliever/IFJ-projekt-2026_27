import ifj26.*

@main def main(): Unit =
  var i = 1
  while i <= 15 do
    if i / 15 * 15 == i then
      print("FizzBuzz")
    else if i / 3 * 3 == i then
      print("Fizz")
    else if i / 5 * 5 == i then
      print("Buzz")
    else
      print(i)
    print(" ")
    i = i + 1
