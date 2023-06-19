/*
 * MedianTemplate.h
 *
 *  Created on: Jun 24, 2022
 *      Author: Yakhontov Alexander yakhontov@gmail.com
 */

#ifndef MEDIANTEMPLATE_H_
#define MEDIANTEMPLATE_H_

#include <Arduino.h>

// Класс для получения медианного занчения
// Предполагается прикрутить его к измерениям аналоговых величин
template <typename T, size_t S> class MedianFilter
{
  private:
    // В value хранятся значения, весь массив отсортирован по возрастанию по этим значениям. В added содержится порядок добавления,
    // у последнего добавленного значение (S-1), у первого добавленного (самого старого) значение 0
    struct {
      T value;
      uint8_t added;
    } data[S];

    const uint8_t medianId = S >> 1;

    void swap(uint8_t one, uint8_t two) { // Меняет в массиве местами два элемента. Аргументы - индексы элементов
      if(one >= S || two >= S)
        return;
      T value = data[one].value;
      uint8_t added = data[one].added;
      data[one].value = data[two].value;
      data[one].added = data[two].added;
      data[two].value = value;
      data[two].added = added;
    }

    void replace(uint8_t id, T value) { // Заменяет в массиве элемент с указанным индексом на новый. Новому устанавливает самый малый возраст
      data[id].value = value;
      data[id].added = S - 1;
    }

    uint8_t findOldest() { // Находит индекс самого старого значения (added == 0), заодно декрементирует все значения added массива
      uint8_t oldest=31;
      for(uint8_t i = 0; i < S; i++)
        if(!(data[i].added--))
          oldest = i;
      return oldest;
    }

    // Находит индекс среди элементов массива от head до tail (head <= tail), куда нужно добавить значение value
    uint8_t find(T value, uint8_t head = 0, uint8_t tail = S-1) {

//      Serial.print("head=");
//      Serial.print(head);
//      Serial.print(" tail=");
//      Serial.print(tail);
//      Serial.print(" value=");
//      Serial.println(value);

      if(head == tail) return head;
      else if(value <= data[head].value) return head;
      else if(value >= data[tail].value) return tail;
      // Когда разница 1 алгоритм зависает, т.к. при целочисленном делении 1 на 2 получается 0 и не
      // меняются условия в следующей итерации
      // Кроме того, возвращать в этом случае head не всегда корректно, в некоторых случаях нужно tail,
      // но для обработки этой ситуации здесь недостаточно данных, поэтому обработаем в другом месте
      else if(tail - head == 1) return head;
      //uint8_t belly = ((int)head + (int)tail) >> 1; if s > if S < 127
      uint8_t belly = (head + tail) >> 1; // if S <= 127
      if(value <= data[belly].value)
        return find(value, head, belly);
      else
        return find(value, belly, tail);
    }

    void move(uint8_t oldest, uint8_t nearest) { // Сдвигает все элементы массива между заданными индексами в сторону старшего с удалением старшего, чтобы освободить место для нового элемента
      if(oldest < nearest)
        for(uint8_t i = oldest; i < nearest; i++){
          data[i].value = data[i+1].value;
          data[i].added = data[i+1].added;
        }
      else
        for(uint8_t i = oldest; i > nearest; i--){
          data[i].value = data[i-1].value;
          data[i].added = data[i-1].added;
        }
    }

  public:
    MedianFilter(T initValue = 0) { reset(initValue); }

    void reset(T initValue = 0) { // Заполняет массив заданным значением
      for(uint8_t i = 0; i < S; i++) {
        data[i].value = initValue;
        data[i].added = i;
      }
    }

    virtual void put(T value) {
      uint8_t oldest = findOldest(); // Находим самый старый элемент массива
      uint8_t nearest = find(value); // Нахоим ближайший по значению элемент массива
      // Если искомый индекс находится между двумя значениями списка, то функция find выдает меньший индекс,
      // что вызывает глюки в функции move. Скорректируем nearest
      if(nearest < S-1 && // Если не конец списка,
        data[nearest].value < value && value < data[nearest+1].value && // нужное значение находится между двумя индексами
        oldest > nearest) // И нужно сдвигать вправо
        nearest++;
      move(oldest, nearest); // Смещаем все элементы таким образом, чтобы удалить самый старый элемент и освободить место в соответствие с сортировкой
      data[nearest].value = value;
      data[nearest].added = S - 1;

//      Serial.print("[ ");
//      for(uint8_t i = 0; i < S; i++) {
//        Serial.print(data[i].value);
//        Serial.print('#');
//        Serial.print(data[i].added);
//        Serial.print(' ');
//      }
//      Serial.print("] oldestId=");
//      Serial.print(oldest);
//      Serial.print(", nearestId=");
//      Serial.print(nearest);
    }

    T get() { return data[medianId].value; }

    T putGet(T value) {
      put(value);
      return get();
    }

    T getMin() { return data[0].value; }

    T getMax() { return data[S-1].value; }
};


#endif /* MEDIANTEMPLATE_H_ */
