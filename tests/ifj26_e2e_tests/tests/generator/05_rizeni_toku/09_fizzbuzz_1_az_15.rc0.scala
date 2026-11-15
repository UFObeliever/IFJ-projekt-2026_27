import ifj26.*

@main def main(): Unit =
  var i = 1
  while i <= 15 do
    val m3 = i - i / 3 * 3
    val m5 = i - i / 5 * 5
    if m3 == 0 then
      if m5 == 0 then
        print("FizzBuzz")
      else
        print("Fizz")
    else
      if m5 == 0 then
        print("Buzz")
      else
        print(i)
    print("\n")
    i = i + 1
